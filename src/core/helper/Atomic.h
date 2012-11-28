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
#include "CheckPointProvider.h"

#define HYFLOW_ATOMIC_START \
HyflowContext* __context__ = ContextManager::getInstance(); \
for (int i = 0; i < 0x7fffffff; i++) { \
	__context__->contextInit(); \
	bool commit = true; \
	try { \

#define HYFLOW_ATOMIC_END \
		} catch (TransactionException & ex) { \
		ex.print();\
		commit = false;\
	}\
	if (commit) {\
		try {\
			__context__->commit();\
			LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");\
			ContextManager::releaseInstance(&__context__);\
			break;\
		} catch (TransactionException & ex) {\
			LOG_DEBUG("XXX-----Transaction Failed Post Validation------XXX\n");\
			ContextManager::releaseInstance(&__context__);\
			ex.print();\
			continue;\
		}\
	}else {\
		LOG_DEBUG("XXX-----Transaction Failed Early Validation------XXX\n");\
		ContextManager::releaseInstance(&__context__);\
		continue;\
	}\
	if ( i == 0x7fffffff -1) {\
		throw new TransactionException("Failed to commit the transaction in the defined retries.");\
	}\
}\

#define HYFLOW_FETCH(ID, ACCESS_TYPE) \
	__context__->fetchObject(ID, ACCESS_TYPE)

#define HYFLOW_ON_READ(ID) \
	__context__->onReadAccess(ID)

#define HYFLOW_ON_WRITE(ID) \
	__context__->onWriteAccess(ID)

#define HYFLOW_PUBLISH_OBJECT(OBJ) \
	__context__->addToPublish(OBJ)

#define HYFLOW_DELETE_OBJECT(OBJ) \
	__context__->addToDelete(OBJ)

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
		// LESSON: Don't increase __context__ scope to function level
		// Execute sub-transactions non-atomically if using check pointing or no nesting
		if ((ContextManager::isContextInit()) &&
				((ContextManager::getNestingModel() == HYFLOW_NO_NESTING) ||
					(ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING))) {
			HyflowContext* __context__ = ContextManager::getInstance();
			atomically(self, args, __context__, retValue);
		}else {
			HYFLOW_ATOMIC_START {
				atomically(self, args, __context__, retValue);
			}HYFLOW_ATOMIC_END;
		}
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
