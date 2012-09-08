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
#include "types/DTL2Context.h"

namespace vt_dstm {

ConcurrentHashMap<unsigned long long, HyflowContext*> ContextManager::contextMap;
boost::shared_mutex ContextManager::clockMutex;
int ContextManager::localNodeClock = 0;

ContextManager::ContextManager() {}

ContextManager::~ContextManager() {}

unsigned long long ContextManager::createTid() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
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

/*
 * LESSON: All of following context related function are directed through contextManager
 * to do meta-data collection or generic stuff required for all type of contexts
 */
void ContextManager::beforeReadAccess(HyflowObject *obj, HyflowContext *context) {

}

HyflowObject* ContextManager::onReadAccess(HyflowObject *obj, HyflowContext *context) {
	return context->onReadAccess(obj);
}

HyflowObject* ContextManager::onWriteAccess(HyflowObject *obj, HyflowContext *context){
	return context->onWriteAccess(obj);
}

void ContextManager::commitContext(HyflowContext *context){

}

void ContextManager::abortContext(HyflowContext *context) {

}

} /* namespace vt_dstm */
