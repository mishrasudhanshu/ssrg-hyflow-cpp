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
#include "../../benchMarks/BenchmarkExecutor.h"

namespace vt_dstm {

ConcurrentHashMap<unsigned long long, HyflowContext*> ContextManager::contextMap;
boost::shared_mutex ContextManager::clockMutex;
// LESSON: As version starts from 0, even after first commit updated version will be 0 i.e. txnClock
int ContextManager::localNodeClock = 1;

ContextManager::ContextManager() {}

ContextManager::~ContextManager() {}

unsigned long long ContextManager::createTid() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*100000 + 0.1*tv.tv_usec + 100*NetworkManager::getNodeId() + BenchmarkExecutor::getThreadId();
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
	std::pair<unsigned long long, HyflowContext*> p;
	p.first = c->getTxnId();
	p.second = c;
	contextMap.insertValue(p);
}

HyflowContext* ContextManager::findContext(unsigned long long int tid) {
	return contextMap.getValue(tid);
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
