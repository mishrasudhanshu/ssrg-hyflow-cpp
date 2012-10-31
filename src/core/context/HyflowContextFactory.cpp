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
	contextStackIndex = 0;
}

HyflowContextFactory::~HyflowContextFactory() {}

HyflowContext* HyflowContextFactory::getContextInstance() {
	HyflowContext* context = NULL;
	if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		if( contextStackIndex==0 ) {
			context = getContextFromStack();
		}else {
			context = contextStack.at(contextStackIndex-1);
		}
		context->increaseContextExecutionDepth();
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		Logger::fatal("HCF : Close nesting not supported currently\n");
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("HCF : Open nesting not supported currently\n");
	}else {
		Logger::fatal("HCF : Invalid Nesting Model\n");
	}
	return context;
}

HyflowContext* HyflowContextFactory::getContextFromStack() {
	if (contextStack.size() > contextStackIndex) {
		LOG_DEBUG("HCF : Providing already created context\n");
		return contextStack.at(contextStackIndex++);
	}

	LOG_DEBUG("HCF : Providing fresh context\n");
	HyflowContext* context = ContextManager::createContext();
	contextStack.push_back(context);
	contextStackIndex++;
	return context;
}

} /* namespace vt_dstm */
