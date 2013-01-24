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

namespace vt_dstm {

ContentionManager::ContentionManager() {}

ContentionManager::~ContentionManager() {}

void ContentionManager::init(void* metaData) {}

// Performs Random back-off if too many aborts are happening
void ContentionManager::deInit(void* metaData) {
	DTL2Context* context = (DTL2Context*)metaData;
	int aborts = context->getAbortCount();
	// Don't perform random back-off in inner Transactions in open nesting which might have abstract locks
	// as they will block all other transactions to proceed too and never back-off internal open
	if (((context->getNestingModel() == HYFLOW_NESTING_OPEN) &&
			context->getParentContext() && ((DTL2Context*)context->getParentContext())->haveAbstractLocks())
			|| (context->getNestingModel() == HYFLOW_INTERNAL_OPEN)) {
		return;
	}
	// Don't perform back-off for other nesting, if explicitly not specified
	if (((context->getNestingModel() == HYFLOW_NESTING_FLAT)
			|| (context->getNestingModel() == HYFLOW_NESTING_CLOSED)
			|| (context->getNestingModel() == HYFLOW_CHECKPOINTING)) &&
			(!BenchmarkExecutor::isBackAllNesting())) {
		return;
	}
	performBackOff(aborts);
}

void ContentionManager::performBackOff(int aborts) {
	int baseBackOff = BenchmarkExecutor::getBaseBackOffTime();
#ifdef RELEASE
	int HYFLOW_BASE_BACKOFF = baseBackOff;
#else
	int HYFLOW_BASE_BACKOFF=5*baseBackOff;
#endif
	int baseSleepTime = HYFLOW_BASE_BACKOFF;
	int sleepTime = aborts*baseSleepTime;
	sleepTime = baseSleepTime + abs(Logger::getCurrentMicroSec())%(sleepTime+1);

	LOG_DEBUG("CONMAN :Performing back-off after %d aborts for %d ms\n", aborts, sleepTime/1000);
	HyflowMetaData sleepMeta;
	sleepMeta.backOffTime = sleepTime/1000;
	BenchmarkExecutor::updateMetaData(sleepMeta, HYFLOW_METADATA_BACKOFF_TIME);
	usleep(sleepTime);
}



} /* namespace vt_dstm */
