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
	if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_FLAT) == 0) {
		nestingModel = HYFLOW_NESTING_FLAT ;
	} else if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_CLOSED) == 0) {
		nestingModel = HYFLOW_NESTING_CLOSED;
	} else if (ConfigFile::Value(NESTING_MODEL).compare(NESTING_OPEN) == 0) {
		nestingModel = HYFLOW_NESTING_OPEN;
	} else {
		Logger::fatal("CM : Unknown Nesting Model\n");
	}
	LOG_DEBUG("CM : Nesting Model %d\n", nestingModel);
}

//TODO: create unique transaction Id, independent of time
unsigned long long ContextManager::createTid() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec%100000000000 + tv.tv_usec)*10000 + 100*NetworkManager::getNodeId() + ThreadMeta::getThreadId();
}

HyflowContext* ContextManager::getInstance() {
	HyflowContextFactory *contextFactory = threadContextFactory.get();
	if (!contextFactory) {
		contextFactory = new HyflowContextFactory();
		threadContextFactory.reset(contextFactory);
	}
	return contextFactory->getContextInstance();
}

void ContextManager::releaseInstance(HyflowContext **c) {
	BenchmarkExecutor::increaseRetries();
	if (*c) {
//		unregisterContext(*c);
		(*c)->contextDeinit();
		*c = NULL;
	}
	HyflowContextFactory *contextFactory = threadContextFactory.get();
	// We will the context using the contextFactory reference
	contextFactory->releaseContextInstance();
}

HyflowContext* ContextManager::createContext() {
	HyflowContext *context = NULL;
	if (ConfigFile::Value(CONTEXT).compare(DTL2) == 0 ) {
		context = new DTL2Context();
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

void ContextManager::registerContext(HyflowContext *c) {
	tbb::concurrent_hash_map<unsigned long long, HyflowContext*>::accessor a;
	if (!contextMap->insert(a,c->getTxnId()))
		throw "Context with same id already exist"+c->getTxnId();
	a->second = c;
}

void ContextManager::unregisterContext(HyflowContext *c) {
	if (!contextMap->erase(c->getTxnId()))
		throw "Context already Unregistered"+c->getTxnId();
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
	localNodeClock++;
}

Hyflow_NestingModel ContextManager::getNestingModel() {
	return nestingModel;
}

void ContextManager::setNestingModel(Hyflow_NestingModel nM) {
	nestingModel = nM;
}

int ContextManager::getClock() {
	return localNodeClock.load();
}

} /* namespace vt_dstm */
