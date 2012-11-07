/*
 * HyflowContextFactory.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdlib>

#include "../../util/logging/Logger.h"
#include "HyflowContextFactory.h"
#include "ContextManager.h"

namespace vt_dstm {

HyflowContextFactory::HyflowContextFactory() {
	contextStackIndex = -1;
	txnIndex = -1;
}

HyflowContextFactory::~HyflowContextFactory() {}

bool HyflowContextFactory::isContextInit() {
	return (contextStackIndex != -1);
}

HyflowContext* HyflowContextFactory::getContextInstance() {
	HyflowContext* context = NULL;
	if  (ContextManager::getNestingModel() == HYFLOW_NO_NESTING) {
		if( contextStackIndex==-1 ) {
			context = getFreshContext();
		}else {
			context = contextStack.at(contextStackIndex);
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		if( contextStackIndex==-1 ) {
			context = getContextFromStack();
		}else {
			context = contextStack.at(contextStackIndex);
			contextStackIndex++;
		}
		context->setContextExecutionDepth(contextStackIndex);
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		txnIndex++;
		// In close nesting we don't check execution depth instead check parent NULL
		// except in context.Init() which checks its depth
		context = getFreshContext();
		if( contextStackIndex != 0 ) {
			context->setParentContext(contextStack[contextStackIndex-1]);
		}
		// Otherwise for top context parent is set to be NULL by default
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("HCF : Open nesting not supported currently\n");
	}else if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		if( contextStackIndex==-1 ) {
			context = getFreshContext();
		}else {
			context = contextStack.at(contextStackIndex);
		}
	}else {
		Logger::fatal("HCF : Invalid Nesting Model\n");
	}
	return context;
}

/*
 *  Perform all cleanUp and free the heap Memory, if required
 */
void HyflowContextFactory::releaseContextInstance(){
	bool throwException = false;
	if  (ContextManager::getNestingModel() == HYFLOW_NO_NESTING) {
		HyflowContext* context = contextStack[0];
		if (context->getStatus() != TXN_ABORTED) {
			contextStackIndex--;
			contextStack.clear();
			ContextManager::deleteContext(&context);
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		HyflowContext* context = contextStack[0];
		// If releasing a instance of aborted transaction, check if we require
		// to throw transaction exception, after context clean-up
		if (context->getStatus() == TXN_ABORTED) {
				LOG_DEBUG("HCF :Performing the checkParent\n");
				throwException = context->checkParent();
				if (throwException) {
					// if we are throwing exception then we should decrease execution depth
					// and stackIndex as control will reach to next level
					context->decreaseContextExecutionDepth();
					contextStackIndex--;
				}
		} else {
			context->decreaseContextExecutionDepth();
			contextStackIndex--;
		}

		if (contextStackIndex == -1){
			// If it was top context free memory if context is not aborted
			LOG_DEBUG(" HCF : Clearing the top context\n");
			contextStack.clear();
			ContextManager::deleteContext(&context);
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		HyflowContext* context = contextStack[contextStackIndex];
		// If releasing a instance of aborted transaction, check if we require
		// to throw transaction exception, after context clean-up
		if (context->getStatus() == TXN_ABORTED) {
				LOG_DEBUG("HCF :Performing the checkParent\n");
				throwException = context->checkParent();
				if (throwException) {
					contextStack.pop_back();
					contextStackIndex--;
					ContextManager::deleteContext(&context);
				}
		}else {
			contextStack.pop_back();
			contextStackIndex--;
			ContextManager::deleteContext(&context);
			if (contextStackIndex == -1) {
				txnIndex = 0;
			}
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("HCF : Open nesting not supported currently\n");
	}else if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HyflowContext* context = contextStack[0];
		if (context->getStatus() != TXN_ABORTED) {
			contextStackIndex--;
			contextStack.clear();
			ContextManager::deleteContext(&context);
		}
	}else {
		Logger::fatal("HCF : Invalid Nesting Model\n");
	}

	if (throwException) {
		LOG_DEBUG("HCF : Check Parent throws an exception\n");
		TransactionException checkParent("Check Parent throws an exception");
		throw checkParent;
	}
}

HyflowContext* HyflowContextFactory::getContextFromStack() {
	if (contextStack.size() > contextStackIndex) {
		LOG_DEBUG("HCF : Providing already created context\n");
		return contextStack.at(contextStackIndex++);
	}
	return getFreshContext();
}

HyflowContext* HyflowContextFactory::getFreshContext() {
	HyflowContext* context = ContextManager::createContext();
	context->setSubTxnIndex(txnIndex);
	contextStack.push_back(context);
	contextStackIndex++;
	LOG_DEBUG("HCF : Providing fresh context\n");
	return context;
}

} /* namespace vt_dstm */
