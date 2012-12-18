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

namespace vt_dstm {

Atomic::Atomic(){
	m_hasOnCommit = false;
	m_hasOnAbort = false;
	atomically = NULL;
	onCommit = Atomic::defaultOnCommit;
	onAbort = Atomic::defaultOnAbort;
	nestingModel = ContextManager::getNestingModel() ;
	arguements = NULL;
	returnType = NULL;
	selfPointer = NULL;
}

Atomic::Atomic(Hyflow_NestingModel model) {
	m_hasOnCommit = false;
	m_hasOnAbort = false;
	atomically = NULL;
	onCommit = defaultOnCommit;
	onAbort = defaultOnAbort;
	nestingModel = model;
	arguements = NULL;
	returnType = NULL;
	selfPointer = NULL;
}

void Atomic::execute(HyflowObject* self, BenchMarkArgs* args, BenchMarkReturn* retValue) {
	// Save argument Values;
	arguements = args;
	returnType = retValue;
	selfPointer = self;

	// LESSON: Don't increase __context__ scope to function level
	// Execute sub-transactions non-atomically if using check pointing or no nesting
	if ((ContextManager::isContextInit()) &&
			((ContextManager::getNestingModel() == HYFLOW_NO_NESTING) ||
				(ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING))) {
		HyflowContext* __context__ = ContextManager::getInstance();
		BenchmarkExecutor::transactionLengthDelay();
		__context__->setCurrentAction(this);
		atomically(self, args, __context__, retValue);
	}else {
		HYFLOW_ATOMIC_START {
			BenchmarkExecutor::transactionLengthDelay();
			atomically(self, args, __context__, retValue);
		}HYFLOW_ATOMIC_END;
	}
}

void Atomic::getClone(Atomic** atomicObject) {
		Atomic *newAtomic = new Atomic();
		newAtomic->m_hasOnCommit = m_hasOnCommit ;
		newAtomic->m_hasOnAbort = m_hasOnAbort;
		newAtomic->atomically = atomically;
		newAtomic->onCommit = onCommit;
		newAtomic->onAbort = onAbort;
		newAtomic->nestingModel = nestingModel;

		newAtomic->arguements = arguements;
		newAtomic->returnType = returnType;
		newAtomic->selfPointer = selfPointer;
		*atomicObject = newAtomic;
}

void Atomic::test() {}

}/* namespace vt_dstm */

