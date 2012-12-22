/*
 * ContextManager.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include <time.h>
#include "ContextManager.h"
#include "../../util/Definitions.h"
#include "../../util/parser/ConfigFile.h"
#include "../../util/logging/Logger.h"
#include "types/DTL2Context.h"
#include "../../util/networking/NetworkManager.h"
#include "../../util/concurrent/ThreadMeta.h"
#include "../../benchMarks/BenchmarkExecutor.h"
#include "../helper/CheckPointProvider.h"

namespace vt_dstm {

boost::thread_specific_ptr<HyflowContextFactory> ContextManager::threadContextFactory;
tbb::concurrent_hash_map<unsigned long long, HyflowContext*>* ContextManager::contextMap;
tbb::atomic<int> ContextManager::localNodeClock;
Hyflow_NestingModel ContextManager::nestingModel;

ContextManager::ContextManager() {}

ContextManager::~ContextManager() {}

void ContextManager::ContextManagerInit() {
// LESSON: As version starts from 0, even after first commit updated version will be 0 i.e. txnClock
	localNodeClock.store(1);
	contextMap = new tbb::concurrent_hash_map<unsigned long long, HyflowContext*>(1024);
	if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_NO) == 0) {
		nestingModel = HYFLOW_NO_NESTING ;
	}else if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_FLAT) == 0) {
		nestingModel = HYFLOW_NESTING_FLAT ;
	} else if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_CLOSED) == 0) {
		nestingModel = HYFLOW_NESTING_CLOSED;
	} else if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_OPEN) == 0) {
		nestingModel = HYFLOW_NESTING_OPEN;
	} else if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_CHECKPOINT) == 0) {
		nestingModel = HYFLOW_CHECKPOINTING ;
		CheckPointProvider::checkPointProviderInit();
	} else{
		Logger::fatal("CM : Unknown Nesting Model\n");
	}
	LOG_DEBUG("CM : Nesting Model %d\n", nestingModel);
}

//TODO: create unique transaction Id, independent of time
unsigned long long ContextManager::createTid(HyflowContext *c) {
	timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec%100000000000 + tv.tv_usec+c->getSubTxnIndex())*10000 + 100*NetworkManager::getNodeId() + ThreadMeta::getThreadId();
}

HyflowContext* ContextManager::getInstance() {
	HyflowContextFactory *contextFactory = threadContextFactory.get();
	if (!contextFactory) {
		contextFactory = new HyflowContextFactory();
		threadContextFactory.reset(contextFactory);
	}
	return contextFactory->getContextInstance();
}

HyflowContext* ContextManager::getInstance(Hyflow_NestingModel nm) {
	HyflowContextFactory *contextFactory = threadContextFactory.get();
	if (!contextFactory) {
		contextFactory = new HyflowContextFactory();
		threadContextFactory.reset(contextFactory);
	}
	return contextFactory->getContextInstance(nm);
}

void ContextManager::releaseInstance(HyflowContext **c) {
	Hyflow_NestingModel nm = (*c)->getNestingModel();
	if (*c) {
		(*c)->contextDeinit();
		// Following part is just for debugging, can be commented
		if ((*c)->getStatus() != TXN_ABORTED) {
			LOG_DEBUG("CM : Transaction committed, reset context pointer to NULL\n");
			*c = NULL;
		}else {
			LOG_DEBUG("CM : Transaction aborted, not resetting context pointer to NULL\n");
			if((*c)->getContextExecutionDepth() == 0 )	//For Top context abort increase retries
				BenchmarkExecutor::increaseRetries();
		}
	}
	HyflowContextFactory *contextFactory = threadContextFactory.get();
	// We will release the context using the contextFactory reference
	contextFactory->releaseContextInstance(nm);
}

HyflowContext* ContextManager::createContext() {
	HyflowContext *context = NULL;
	if (ConfigFile::Value(CONTEXT).compare(DTL2) == 0 ) {
		context = new DTL2Context();
	}
	return context;
}

HyflowContext* ContextManager::createContext(Hyflow_NestingModel nm) {
	HyflowContext *context = NULL;
	if (ConfigFile::Value(CONTEXT).compare(DTL2) == 0 ) {
		context = new DTL2Context(nm);
	}
	return context;
}

void ContextManager::deleteContext(HyflowContext **c) {
	if (*c) {
		HyflowContext* saveContext = *c;
		*c = NULL;
		delete saveContext;
	}
}

bool ContextManager::isContextInit() {
	HyflowContextFactory *contextFactory = threadContextFactory.get();
	if (!contextFactory) {
		return false;
	}
	return contextFactory->isContextInit();
}
void ContextManager::registerContext(HyflowContext *c) {
	tbb::concurrent_hash_map<unsigned long long, HyflowContext*>::accessor a;
	if (!contextMap->insert(a,c->getTxnId())) {
		Logger::fatal("Context with same id already exist %llu \n",c->getTxnId());
		throw "Context with same id already exist"+c->getTxnId();
	}
	a->second = c;
}

void ContextManager::unregisterContext(HyflowContext *c) {
	if (!contextMap->erase(c->getTxnId())) {
		Logger::fatal("Context already Unregistered %llu \n",c->getTxnId());
		throw "Context already Unregistered"+c->getTxnId();
	}
}

HyflowContext* ContextManager::findContext(const unsigned long long tid) {
	HyflowContext* context = NULL;
	{
		tbb::concurrent_hash_map<unsigned long long, HyflowContext*>::const_accessor a;
		if (contextMap->find(a,tid)) {
			context = a->second;
			if(!context) {
				throw "NULL Context Found!!";
			}
			return context;
		} else {
			throw "No Context Found!!";
		}
	}
	return context;
}

bool ContextManager::atomicUpdateClock(int newClock, int oldClock) {
	int oldValue = localNodeClock.compare_and_swap(newClock, oldClock);
	if (oldValue == oldClock) {
		LOG_DEBUG("CLOCK :Node clock moved from %d to %d\n", oldClock, newClock);
		localNodeClock = newClock;
		return true;
	}else {
		return false;
	}
}

void ContextManager::atomicIncreaseClock() {
	int old = localNodeClock.load();
	localNodeClock++;
	LOG_DEBUG("CM :Move clock from %d to %d\n", old, localNodeClock.load());
}

Hyflow_NestingModel ContextManager::getNestingModel() {
	return nestingModel;
}

void ContextManager::setNestingModel(Hyflow_NestingModel nM) {
	nestingModel = nM;
}

int ContextManager::getClock() {
	int clock = localNodeClock.load();
	LOG_DEBUG("CM :Current clock %d\n", clock);
	return clock;
}

} /* namespace vt_dstm */
