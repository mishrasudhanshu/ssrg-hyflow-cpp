/*
 * ContextManager.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include "ContextManager.h"
#include "../../util/Definitions.h"
#include "../../util/parser/ConfigFile.h"
#include "types/DTL2Context.h"

namespace vt_dstm {

ConcurrentHashMap<unsigned long long, HyflowContext*> ContextManager::contextMap;

ContextManager::ContextManager() {}

ContextManager::~ContextManager() {}

HyflowContext* ContextManager::getInstance() {
	HyflowContext *context = NULL;
	// Create correct context and register it
	if (ConfigFile::Value(CONTEXT).compare(DTL2) == 0 ) {
		context = new DTL2Context();
	}
	return context;
}

HyflowContext* ContextManager::findContext(unsigned long long int tid) {
	return contextMap.getValue(tid);
}

} /* namespace vt_dstm */
