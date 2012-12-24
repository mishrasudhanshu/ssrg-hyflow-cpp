/*
 * LoanBenchMark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "LoanBenchMark.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../util/concurrent/ThreadMeta.h"
#include <vector>

namespace vt_dstm {

LoanBenchmark::LoanBenchmark() {}

LoanBenchmark::~LoanBenchmark() {
	delete ids;
}

int LoanBenchmark::getOperandsCount() {
	return BenchmarkExecutor::getObjectNesting();
}

void LoanBenchmark::readOperation(std::string ids[], int size) {
	LOG_DEBUG("Sum of Loan Accounts\n");
	std::vector<std::string> accounts;
	for(int i=0 ; i<size ; i++ ) {
		accounts.push_back(ids[i]);
	}
	LoanAccount::sum(accounts);
}

void LoanBenchmark::writeOperation(std::string ids[], int size) {
	int a = 100;
	a += ThreadMeta::getThreadId();	// Only for debug sanity
	LOG_DEBUG("Loan on accounts\n");
	std::vector<std::string> accounts;
	for(int i=1 ; i<size ; i++ ) {
		accounts.push_back(ids[i]);
	}
	LoanAccount::borrow(ids[0], accounts, a, true);
}

void LoanBenchmark::checkSanity() {
	sleep(2);
}

std::string* LoanBenchmark::createLocalObjects(int objCount) {
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
			LoanAccount la(AMOUNT, ids[i]);
			DirectoryManager::registerObjectWait(&la, 0);
		}
	}
	return ids;
}

} /* namespace vt_dstm */
