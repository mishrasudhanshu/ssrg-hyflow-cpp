/*
 * ContextManager.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CONTEXTMANAGER_H_
#define CONTEXTMANAGER_H_

#include <boost/thread/shared_mutex.hpp>

#include "HyflowContext.h"
#include "../exceptions/types/TransactionException.h"
#include "../../util/concurrent/ConcurrentHashMap.h"

namespace vt_dstm {

class ContextManager {
	static ConcurrentHashMap<unsigned long long, HyflowContext*> contextMap;

	static boost::shared_mutex clockMutex;
	static int localNodeClock;
	static unsigned long long createTid();
public:
	ContextManager();
	virtual ~ContextManager();

	static void ContextManagerInit();
	static HyflowContext* getInstance();
	static void registerContext(HyflowContext * c);
	static HyflowContext* findContext(unsigned long long tid);
	/**
	 * To update the clock value on arrival of message from newer clock node
	 * returns false if atomic compare and set was unsuccessful
	 */
	static bool atomicUpdateClock(int newClock, int oldClock);
	/**
	 * To atomically increase clock on transaction completion
	 */
	static void atomicIncreaseClock();
	/**
	 * To access the current clock value of node
	 */
	static int getClock();
};

} /* namespace vt_dstm */

#endif /* CONTEXTMANAGER_H_ */
