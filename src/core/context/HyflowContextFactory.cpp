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
}

HyflowContextFactory::~HyflowContextFactory() {}

HyflowContext* HyflowContextFactory::getContextInstance() {
	HyflowContext* context = NULL;
	if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		if( contextStackIndex==-1 ) {
			context = getContextFromStack();
		}else {
			context = contextStack.at(contextStackIndex);
			contextStackIndex++;
		}
		context->setContextExecutionDepth(contextStackIndex);
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		// In close nesting we don't check execution depth instead check parent NULL
		context = getFreshContext();
		if( contextStackIndex != 0 ) {
			context->setParentContext(contextStack[contextStackIndex-1]);
		}
		// Otherwise for top context parent is set to be NULL by default
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("HCF : Open nesting not supported currently\n");
	}else {
		Logger::fatal("HCF : Invalid Nesting Model\n");
	}
	return context;
}

void HyflowContextFactory::releaseContextInstance(){
	bool throwException = false;

	// Perform all cleanUp and free the heap Memory
	if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		HyflowContext* context = contextStack[0];
		// If releasing a instance of aborted transaction, check if we require
		// to throw transaction exception, after context clean-up
		if (context->getStatus() == TXN_ABORTED) {
				throwException = context->checkParent();
				LOG_DEBUG("HCF :Performing the checkParent\n");
		}
		context->decreaseContextExecutionDepth();
		contextStackIndex--;
		if (contextStackIndex == -1) {	// If it was top context free memory
			LOG_DEBUG(" HCF : Clearing the top context\n");
			contextStack.clear();
			ContextManager::deleteContext(&context);
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		HyflowContext* context = contextStack[contextStackIndex];
		// If releasing a instance of aborted transaction, check if we require
		// to throw transaction exception, after context clean-up
		if (context->getStatus() == TXN_ABORTED) {
				throwException = context->checkParent();
		}
		contextStack.pop_back();
		contextStackIndex--;
		ContextManager::deleteContext(&context);
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("HCF : Open nesting not supported currently\n");
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
	contextStack.push_back(context);
	contextStackIndex++;
	LOG_DEBUG("HCF : Providing fresh context\n");
	return context;
}

} /* namespace vt_dstm */
