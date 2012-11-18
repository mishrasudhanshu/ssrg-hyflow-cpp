/*
 * TPCCBenchmark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TPCCBenchmark.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../util/concurrent/ThreadMeta.h"

namespace vt_dstm {

TPCCBenchmark::TPCCBenchmark() {}

TPCCBenchmark::~TPCCBenchmark() {
	delete ids;
}

int TPCCBenchmark::getOperandsCount() {
	return 2;
}

void TPCCBenchmark::readOperation(std::string ids[], int size) {
	TpccOperation();
}

void TPCCBenchmark::writeOperation(std::string ids[], int size) {
	TpccOperation();
}

void TPCCBenchmark::TpccOperation() {
	int opt = abs(Logger::getCurrentMicroSec())%100;
	if (opt < 4) {
			LOG_DEBUG("Run Transaction: Order Status");
			TPCC_Ops::orderStatus();
	} else if (opt < 8) {
			LOG_DEBUG("Run Transaction: Delivery");
			TPCC_Ops::delivery();
	} else if (opt < 12) {
			LOG_DEBUG("Run Transaction: Stock Level");
			TPCC_Ops::stockLevel();
	} else if (opt < 55) {
			LOG_DEBUG("Run Transaction: Payment");
			TPCC_Ops::payment();
	} else {
			LOG_DEBUG("Run Transaction: New Order");
			TPCC_Ops::newOrder();
	}
}

void TPCCBenchmark::checkSanity() {}

std::string* TPCCBenchmark::createLocalObjects(int objCount) {
	objectCount = objCount;
	ids = new std::string [objCount];
	int nodeCount =  NetworkManager::getNodeCount();
	int nodeId = NetworkManager::getNodeId();
	for(int i=0; i<objCount ; i++){
		std::ostringstream idStream;
		idStream << i%nodeCount <<"-"<< i;
		ids[i] = idStream.str();
		if((i % nodeCount)== nodeId ){
			LOG_DEBUG("Created locally object %d\n", i);
			// Create a stack copy of object
			// register account will create a heap copy of object
			// and save its pointer in local cache
			TPCC ba(AMOUNT, ids[i]);
			DirectoryManager::registerObjectWait(&ba, 0);
		}
	}
	return ids;
}

} /* namespace vt_dstm */
