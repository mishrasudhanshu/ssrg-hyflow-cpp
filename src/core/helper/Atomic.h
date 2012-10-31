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
		nestingModel = HYFLOW_NESTING_FLAT ;
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
			}catch (...) {
				try {
					ContextManager::cleanInstance(&c);
					throw;
				} catch (TransactionException & ex) {
					ex.print();
					commit = false;
				} catch (std::string & s) {
					Logger::fatal("%s\n",s.c_str());
					return;
				}
			}
			if (commit) {
				try {
					c->commit();
					LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");
					ContextManager::cleanInstance(&c);
					return;
				} catch(...) {
					try{
						ContextManager::cleanInstance(&c);
						throw;
					} catch (TransactionException & ex) {
						ex.print();
						continue;
					} catch (std::string & s) {
						Logger::fatal("%s\n",s.c_str());
						return;
					}
				}
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
