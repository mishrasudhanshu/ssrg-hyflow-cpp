/*
 * Atomic.cpp
 *
 *  Created on: Dec 18, 2012
 *      Author: mishras[at]vt.edu
 */

#include "Atomic.h"
#include "../../util/logging/Logger.h"
#include "../../core/HyflowObject.h"
#include "../../core/context/ContextManager.h"
#include "../../benchMarks/BenchmarkExecutor.h"

#define HYFLOW_ATOMIC_START_INTERNAL \
	HyflowContext* __context__ = ContextManager::getInstance(nestingModel); \
	for (int __hyflow_attempt__ = 0; __hyflow_attempt__ < 0x7fffffff; __hyflow_attempt__++) { \
		__context__->contextInit(); \
		bool __hyflow_commit__ = true; \
		BenchmarkExecutor::transactionLengthDelay(); \
		try { \

namespace vt_dstm {

Atomic::Atomic(){
	atomically = NULL;
	onCommit = Atomic::defaultOnCommit;
	onAbort = Atomic::defaultOnAbort;
	nestingModel = ContextManager::getNestingModel() ;
	arguements = NULL;
	returnValue = NULL;
	selfPointer = NULL;
	completed = false;
	onHeap = false;
}

Atomic::Atomic(Hyflow_NestingModel model) {
	atomically = NULL;
	onCommit = defaultOnCommit;
	onAbort = defaultOnAbort;
	nestingModel = model;
	arguements = NULL;
	returnValue = NULL;
	selfPointer = NULL;
	completed = false;
	onHeap = false;
}

Atomic::~Atomic() {
	if ( nestingModel == HYFLOW_NESTING_OPEN ) {
		if (isOnHeap()) {
			delete arguements;
			delete returnValue;
		}
	}
}

void Atomic::execute(HyflowObject* self, BenchMarkArgs* args, BenchMarkReturn* retValue) {
	// Save argument Values;
	arguements = args;
	returnValue = retValue;
	selfPointer = self;

	// LESSON: Don't increase __context__ scope to function level
	// Execute sub-transactions non-atomically if using check pointing or no nesting
	if ((ContextManager::isContextInit()) &&
			((nestingModel == HYFLOW_NO_NESTING) ||
				(nestingModel == HYFLOW_CHECKPOINTING))) {
		HyflowContext* __context__ = ContextManager::getInstance(nestingModel);
		BenchmarkExecutor::transactionLengthDelay();
		atomically(self, args, __context__, retValue);
	}else if (nestingModel == HYFLOW_INTERNAL_OPEN) {
		HyflowMetaData inTxn;
		unsigned long long start = Logger::getCurrentMicroSec();
		HYFLOW_ATOMIC_START_INTERNAL {
			BenchmarkExecutor::transactionLengthDelay();
			atomically(self, args, __context__, retValue);
		}HYFLOW_ATOMIC_END;
		inTxn.compensateSubTxnTime = Logger::getCurrentMicroSec() - start;
		BenchmarkExecutor::updateMetaData(inTxn, HYFLOW_METADATA_COMPENSATE_SUBTXN_TIME);
		BenchmarkExecutor::increaseMetaData(HYFLOW_METADATA_COMPENSATE_SUBTXNS);
	}else {
		HYFLOW_ATOMIC_START {
			BenchmarkExecutor::transactionLengthDelay();
			atomically(self, args, __context__, retValue);

			if ( nestingModel == HYFLOW_NESTING_OPEN ) {
				Atomic* actionCopy = NULL;
				getClone(&actionCopy);
				__context__->setCurrentAction(actionCopy);
			}
		}HYFLOW_ATOMIC_END;
	}
}

void Atomic::getClone(Atomic** atomicObject) {
		Atomic *newAtomic = new Atomic();
		newAtomic->completed = completed ;
		newAtomic->atomically = atomically;
		newAtomic->onCommit = onCommit;
		newAtomic->onAbort = onAbort;
		newAtomic->nestingModel = nestingModel;

		if (arguements)
			arguements->getClone(&(newAtomic->arguements));
		if (returnValue)
			returnValue->getClone(&(newAtomic->returnValue));

		newAtomic->selfPointer = selfPointer;
		newAtomic->onHeap = true;
		*atomicObject = newAtomic;
}

void Atomic::test() {}

}/* namespace vt_dstm */

