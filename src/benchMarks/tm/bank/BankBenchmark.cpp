/*
 * BankBenchmark.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#include "BankBenchmark.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../util/concurrent/ThreadMeta.h"

namespace vt_dstm {

BankBenchmark::BankBenchmark() {}

BankBenchmark::~BankBenchmark() {
	delete ids;
}

int BankBenchmark::getOperandsCount() {
	return 2*2;
}

void BankBenchmark::readOperation(std::string ids[], int size) {
	for(int txns=0 ; txns<size ; txns+=2 ) {
		LOG_DEBUG("Total Balance of %s & %s\n", ids[txns].c_str(), ids[txns+1].c_str());
	}
	BankAccount::totalBalanceMulti(ids, size);
}

void BankBenchmark::writeOperation(std::string ids[], int size) {
	int a = 1;
	a += ThreadMeta::getThreadId();	// Only for debug sanity
	for(int txns=0 ; txns<size ; txns+=2 ) {
		LOG_DEBUG("Transfer from %s to %s %d\n", ids[txns].c_str(), ids[txns+1].c_str(), a);
	}
	BankAccount::transferMulti(ids, size, a);
}

void BankBenchmark::checkSanity() {
	sleep(2);
	BankAccount::checkSanity(ids, objectCount);
}

std::string* BankBenchmark::createLocalObjects(int objCount) {
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
			BankAccount ba(AMOUNT, ids[i]);
			DirectoryManager::registerObjectWait(&ba, 0);
		}
	}
	return ids;
}

} /* namespace vt_dstm */
