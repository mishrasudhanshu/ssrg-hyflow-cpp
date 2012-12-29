/*
 * Atomic.h
 *
 *  Created on: Oct 19, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <cstdlib>
#include "BenchMarkArgs.h"
#include "BenchMarkReturn.h"
#include "CheckPointProvider.h"

#define HYFLOW_ATOMIC_START \
HyflowContext* __context__ = ContextManager::getInstance(); \
for (int __hyflow_attempt__ = 0; __hyflow_attempt__ < 0x7fffffff; __hyflow_attempt__++) { \
	__context__->contextInit(); \
	bool __hyflow_commit__ = true; \
	BenchmarkExecutor::transactionLengthDelay(); \
	try { \

#define HYFLOW_ATOMIC_END \
	} catch (TransactionException & ex) { \
		ex.print();\
		__hyflow_commit__ = false;\
	}\
	if (__hyflow_commit__) {\
		try {\
			__context__->commit();\
			if (__context__->getContextExecutionDepth() == 0) {	\
				LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n"); \
			}else { \
				LOG_DEBUG(".....InnerTxn Successful.....\n");\
			} \
			ContextManager::releaseInstance(&__context__);\
			break;\
		} catch (TransactionException & ex) {\
			if (__context__->getContextExecutionDepth() == 0) {	\
				LOG_DEBUG("XXX-----Transaction Failed Post Validation------XXX\n");\
			}else { \
				LOG_DEBUG("xxxx-InnerTxn Failed Post Validation-xxxx\n");\
			} \
			ContextManager::releaseInstance(&__context__);\
			ex.print();\
			continue;\
		}\
	}else {\
		if (__context__->getContextExecutionDepth() == 0) {	\
			LOG_DEBUG("XXX-----Transaction Failed Early Validation------XXX\n");\
		}else { \
			LOG_DEBUG("xxxx-InnerTxn Early Failed Validation-xxxx\n");\
		} \
		ContextManager::releaseInstance(&__context__);\
		continue;\
	}\
	if ( __hyflow_attempt__ == 0x7fffffff -1) {\
		throw new TransactionException("Failed to commit the transaction in the defined retries.");\
	}\
}\

#define HYFLOW_FETCH(ID, ACCESS_TYPE) \
	__context__->fetchObject(ID, ACCESS_TYPE)

#define HYFLOW_NFETCH(ID, ACCESS_TYPE, ABORT_ON_NULL) \
	__context__->fetchObject(ID, ACCESS_TYPE, ABORT_ON_NULL)

#define HYFLOW_ON_READ(ID) \
	__context__->onReadAccess(ID)

#define HYFLOW_ON_WRITE(ID) \
	__context__->onWriteAccess(ID)

#define HYFLOW_PUBLISH_OBJECT(OBJ) \
	__context__->addToPublish(OBJ)

#define HYFLOW_DELETE_OBJECT(OBJ) \
	__context__->addToDelete(OBJ)

#define HYFLOW_LOCATE_OBJECT(ID) \
	__context__->locateObject(ID)

namespace vt_dstm {

/*
 * Other way to do it can be using explicit overloading void type for Atomic class
 */
class Atomic {
	Hyflow_NestingModel nestingModel;
	bool completed;	// To suggest whether belonging transaction got committed
	bool onHeap;

	BenchMarkArgs* arguements;
	BenchMarkReturn* returnValue;
	HyflowObject* selfPointer;


	static void defaultOnCommit(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* rt) {}
	static void defaultOnAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* rt) {}
	static HyflowObject* locateObject(std::string objectId);
public:
	void (*atomically)(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* rt);

	void (*onCommit)(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* rt);
	void (*onAbort)(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* rt);

	Atomic();

	Atomic(Hyflow_NestingModel model);
	virtual ~Atomic();

	void execute(HyflowObject* self, BenchMarkArgs* args, BenchMarkReturn* retValue);

	void getClone(Atomic** atomicObject);
	static void test();

	bool isOnHeap() const {
		return onHeap;
	}

	void setOnHeap(bool onHeap) {
		this->onHeap = onHeap;
	}

	bool isCompleted() const {
		return completed;
	}

	void setCompleted(bool completed) {
		this->completed = completed;
	}

	BenchMarkArgs* getArguements() const {
		return arguements;
	}

	void setArguements(BenchMarkArgs* arguements) {
		this->arguements = arguements;
	}

	BenchMarkReturn* getReturnValue() const {
		return returnValue;
	}

	void setReturnValue(BenchMarkReturn* returnValue) {
		this->returnValue = returnValue;
	}
};

} /* namespace vt_dstm */

#endif /* ATOMIC_H_ */
