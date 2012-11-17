/*
 * VacationBenchmark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "VacationBenchmark.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../util/concurrent/ThreadMeta.h"

namespace vt_dstm {

VacationBenchmark::VacationBenchmark() {}

VacationBenchmark::~VacationBenchmark() {
	delete ids;
}

int VacationBenchmark::getOperandsCount() {
	return 2;
}

void VacationBenchmark::readOperation(std::string ids[], int size) {
	LOG_DEBUG("Total Balance of %s & %s\n", ids[0].c_str(), ids[1].c_str());
	BankAccount::totalBalance(ids[0], ids[1]);
}

void VacationBenchmark::writeOperation(std::string ids[], int size) {
	int a = 1;
	a += ThreadMeta::getThreadId();	// Only for debug sanity
	LOG_DEBUG("Transfer from %s to %s %d\n", ids[0].c_str(), ids[1].c_str(), a);
	BankAccount::transfer(ids[0], ids[1], a);
}

void VacationBenchmark::checkSanity() {
	sleep(2);
	BankAccount::checkSanity(ids, objectCount);
}

std::string* VacationBenchmark::createLocalObjects(int objCount) {
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
