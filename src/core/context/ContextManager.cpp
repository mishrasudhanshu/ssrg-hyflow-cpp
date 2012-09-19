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
#include "../../util/concurrent/ThreadId.h"

namespace vt_dstm {

tbb::concurrent_hash_map<unsigned long long, HyflowContext*> ContextManager::contextMap;
boost::shared_mutex ContextManager::clockMutex;
// LESSON: As version starts from 0, even after first commit updated version will be 0 i.e. txnClock
int ContextManager::localNodeClock = 1;

ContextManager::ContextManager() {}

ContextManager::~ContextManager() {}

unsigned long long ContextManager::createTid() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*100000 + 0.1*tv.tv_usec + 100*NetworkManager::getNodeId() + ThreadId::getThreadId();
}

HyflowContext* ContextManager::getInstance() {
	HyflowContext *context = NULL;
	// Create correct context and register it
	if (ConfigFile::Value(CONTEXT).compare(DTL2) == 0 ) {
		context = new DTL2Context();
	}
	context->setTxnId(createTid());
	registerContext(context);
	return context;
}

void ContextManager::registerContext(HyflowContext *c) {
	tbb::concurrent_hash_map<unsigned long long, HyflowContext*>::accessor a;
	if (!contextMap.insert(a,c->getTxnId())) {
		throw "Context with same transaction Id already exist";
	}
	a->second = c;
}

HyflowContext* ContextManager::findContext(const unsigned long long tid) {
	HyflowContext* context = NULL;
	{
		tbb::concurrent_hash_map<unsigned long long, HyflowContext*>::const_accessor a;
		if (contextMap.find(a,tid)) {
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
	boost::upgrade_lock<boost::shared_mutex> writeLock(clockMutex);
	if (localNodeClock == oldClock) {
		LOG_DEBUG("CLOCK :Node clock moved from %d to %d\n", oldClock, newClock);
		localNodeClock = newClock;
		return true;
	}else {
		return false;
	}
}

void ContextManager::atomicIncreaseClock() {
	boost::upgrade_lock<boost::shared_mutex> writeLock(clockMutex);
	localNodeClock++;
}

int ContextManager::getClock() {
	boost::shared_lock<boost::shared_mutex> readLock(clockMutex);
	return localNodeClock;
}

} /* namespace vt_dstm */
