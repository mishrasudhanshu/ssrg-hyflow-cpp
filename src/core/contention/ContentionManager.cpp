/*
 * ContentionManager.cpp
 *
 *  Created on: Dec 30, 2012
 *      Author: mishras[at]vt.edu
 */

#include "ContentionManager.h"

#include <cstdlib>
#include <unistd.h>

#include "../../util/logging/Logger.h"
#include "../../util/networking/NetworkManager.h"
#include "../context/types/DTL2Context.h"
#include "../../benchMarks/BenchmarkExecutor.h"

#define HYFLOW_BACKOFF_BARRIER 9

#ifdef RELEASE
#define HYFLOW_BASE_BACKOFF 2000
#else
#define HYFLOW_BASE_BACKOFF 10000
#endif

namespace vt_dstm {

ContentionManager::ContentionManager() {}

ContentionManager::~ContentionManager() {}

void ContentionManager::init(void* metaData) {}

// Performs Random back-off if too many aborts are happening
void ContentionManager::deInit(void* metaData) {
	DTL2Context* context = (DTL2Context*)metaData;
	// Don't perform random back-off in inner Transactions, in open nesting they might have
	// abstract locks which will block all other transactions to proceed too. Never back-off internal open
	if (!context->getParentContext() && (context->getNestingModel() != HYFLOW_INTERNAL_OPEN)) {
		int aborts = context->getAbortCount();
		int baseSleepTime = HYFLOW_BASE_BACKOFF*NetworkManager::getThreadCount()*NetworkManager::getNodeCount()*BenchmarkExecutor::getInnerTxns();
		int sleepTime = ((aborts*aborts*aborts)/HYFLOW_BACKOFF_BARRIER)*baseSleepTime;
		sleepTime = sleepTime + abs(Logger::getCurrentMicroSec())%(sleepTime+1);
		// Do random back-Off
		if (sleepTime && context->isIsWrite()) {
			LOG_DEBUG("CONMAN :Performing back-off after %d aborts for %d ms\n", aborts, sleepTime/1000);
			usleep(sleepTime);
		}
	}
}

} /* namespace vt_dstm */
