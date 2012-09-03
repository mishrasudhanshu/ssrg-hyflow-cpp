/*
 * ContextManager.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CONTEXTMANAGER_H_
#define CONTEXTMANAGER_H_

#include "HyflowContext.h"
#include "../exceptions/types/TransactionException.h"
#include "../../util/concurrent/ConcurrentHashMap.h"

namespace vt_dstm {

class ContextManager {
	static ConcurrentHashMap<unsigned long long, HyflowContext*> contextMap;
public:
	ContextManager();
	virtual ~ContextManager();

	static void ContextManagerInit();
	static HyflowContext* getInstance();
	static HyflowContext* findContext(unsigned long long tid);
};

} /* namespace vt_dstm */

#endif /* CONTEXTMANAGER_H_ */
