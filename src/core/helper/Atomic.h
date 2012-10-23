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

namespace vt_dstm {

enum Hyflow_NestingModel {
	HYFLOW_NESTING_FLAT,
	HYFLOW_NESTING_CLOSED,
	HYFLOW_NESTING_OPEN
};

template <class T>
class Atomic {
	Hyflow_NestingModel nestingModel = HYFLOW_NESTING_FLAT ;
	bool m_hasOnCommit;
	bool m_hasOnAbort;

	void defaultOnCommit(std::string selfID, void* args, HyflowContext* context) {}
	void defaultOnAbort(std::string selfID, void* args, HyflowContext* context) {}
public:
	T (*atomically)(std::string selfID, void* args, HyflowContext* context);

	void (*onCommit)(std::string selfID, void* args, HyflowContext* context);
	void (*onAbort)(std::string selfID, void* args, HyflowContext* context);

	Atomic(){
		m_hasOnCommit = false;
		m_hasOnAbort = false;
		atomically = NULL;
		onCommit = defaultOnCommit;
		onAbort = defaultOnAbort;
	}

	virtual ~Atomic() {}

	T execute(std::string selfID, void* args) {
		T retValue;
		for (int i = 0; i < 0x7fffffff; i++) {
			bool commit = true;
			HyflowContext* c = ContextManager::getInstance();
			try {
				atomically(selfID, args, c);
			}catch (...) {
				try {
					ContextManager::cleanInstance(&c);
					throw;
				} catch (TransactionException & ex) {
					ex.print();
					commit = false;
				} catch (std::string & s) {
					Logger::fatal("%s\n",s.c_str());
					return retValue;
				}
			}
			if (commit) {
				try {
					c->commit();
					LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");
					ContextManager::cleanInstance(&c);
					return retValue;
				} catch(...) {
					try{
						ContextManager::cleanInstance(&c);
						throw;
					} catch (TransactionException & ex) {
						ex.print();
						continue;
					} catch (std::string & s) {
						Logger::fatal("%s\n",s.c_str());
						return retValue;
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
		Atomic<bool> * testAtomic = new Atomic<bool>();
	}

};

} /* namespace vt_dstm */

#endif /* ATOMIC_H_ */
