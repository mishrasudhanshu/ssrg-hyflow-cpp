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

namespace vt_dstm {

BankBenchmark::BankBenchmark() {
	// TODO Auto-generated constructor stub

}

BankBenchmark::~BankBenchmark() {
	delete ids;
}

int BankBenchmark::getOperandsCount() {
	return 2;
}

void BankBenchmark::readOperation(std::string ids[], int size) {
	LOG_DEBUG("Total Balance of %s & %s\n", ids[0].c_str(), ids[1].c_str());
	BankAccount::totalBalance(ids[0], ids[1]);
}

void BankBenchmark::writeOperation(std::string ids[], int size) {
	static int a = 1;
	a += BenchmarkExecutor::getThreadId();	// Only for debug sanity
	LOG_DEBUG("Transfer from %s to %s %d\n", ids[0].c_str(), ids[1].c_str(), a);
	BankAccount::transfer(ids[0], ids[1], a);
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
			BankAccount ba(10000, ids[i]);
			DirectoryManager::registerObjectWait(ba, 0);
		}
	}
	return ids;
}

} /* namespace vt_dstm */
