/*
 * Atomic.h
 *
 *  Created on: Oct 19, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <cstdlib>
#include "../../util/logging/Logger.h"
#include "../../core/HyflowObject.h"
#include "../../core/context/ContextManager.h"

namespace vt_dstm {

/*
 * Other way to do it can be using explicit overloading void type for Atomic class
 */
template <class ReturnType>
class Atomic {
	Hyflow_NestingModel nestingModel;
	bool m_hasOnCommit;
	bool m_hasOnAbort;

	static void defaultOnCommit(HyflowObject* self, void* args, HyflowContext* context) {}
	static void defaultOnAbort(HyflowObject* self, void* args, HyflowContext* context) {}
public:
	void (*atomically)(HyflowObject* self, void* args, HyflowContext* context, ReturnType* rt);

	void (*onCommit)(HyflowObject* self, void* args, HyflowContext* context);
	void (*onAbort)(HyflowObject* self, void* args, HyflowContext* context);

	Atomic(){
		m_hasOnCommit = false;
		m_hasOnAbort = false;
		atomically = NULL;
		onCommit = Atomic::defaultOnCommit;
		onAbort = Atomic::defaultOnAbort;
		nestingModel = ContextManager::getNestingModel() ;
	}

	Atomic(Hyflow_NestingModel model) {
		m_hasOnCommit = false;
		m_hasOnAbort = false;
		atomically = NULL;
		onCommit = defaultOnCommit;
		onAbort = defaultOnAbort;
		nestingModel = model;
	}

	virtual ~Atomic() {}

	void execute(HyflowObject* self, void* args, ReturnType* retValue) {
		HyflowContext* c = ContextManager::getInstance();
		for (int i = 0; i < 0x7fffffff; i++) {
			c->contextInit();
			bool commit = true;
			try {
				atomically(self, args, c, retValue);
			} catch (TransactionException & ex) {
				ex.print();
				commit = false;
			}
			if (commit) {
				try {
					c->commit();
					LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");
					ContextManager::releaseInstance(&c);
					return;
				} catch (TransactionException & ex) {
					LOG_DEBUG("XXX-----Transaction Failed Post Validation------XXX\n");
					ContextManager::releaseInstance(&c);
					ex.print();
					continue;
				}
			}else {
				LOG_DEBUG("XXX-----Transaction Failed Early Validation------XXX\n");
				ContextManager::releaseInstance(&c);
				continue;
			}
		}
		throw new TransactionException("Failed to commit the transaction in the defined retries.");
	}

	bool hasOnCommit() {
		return m_hasOnCommit;
	}

	bool hasOnAbort() {
		return m_hasOnAbort;
	}

	static void test() {
		Atomic<bool>* testBool = new Atomic<bool>();
		Atomic<uint64_t>* testUint64 = new Atomic<uint64_t>();
	}

};

} /* namespace vt_dstm */

#endif /* ATOMIC_H_ */
