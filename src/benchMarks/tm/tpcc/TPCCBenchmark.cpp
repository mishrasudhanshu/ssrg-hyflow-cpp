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
	LOG_DEBUG("Total Balance of %s & %s\n", ids[0].c_str(), ids[1].c_str());
	TPCC::totalBalance(ids[0], ids[1]);
}

void TPCCBenchmark::writeOperation(std::string ids[], int size) {
	int a = 1;
	a += ThreadMeta::getThreadId();	// Only for debug sanity
	LOG_DEBUG("Transfer from %s to %s %d\n", ids[0].c_str(), ids[1].c_str(), a);
	TPCC::transfer(ids[0], ids[1], a);
}

void TPCCBenchmark::checkSanity() {
	sleep(2);
	TPCC::checkSanity(ids, objectCount);
}

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
