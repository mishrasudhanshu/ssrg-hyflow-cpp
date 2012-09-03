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
	BankAccount::totalBalance(ids[0], ids[1]);
}

void BankBenchmark::writeOperation(std::string ids[], int size) {
	BankAccount::transfer(ids[0], ids[1], 10);
}

void BankBenchmark::checkSanity() {

}

std::string* BankBenchmark::createLocalObjects(int objCount) {
	ids = new std::string [objCount];
	int nodeCount =  NetworkManager::getNodeCount();
	int nodeId = NetworkManager::getNodeId();
	for(int i=0; i<objCount ; i++){
		std::ostringstream idStream;
		idStream << nodeId <<"-"<< i;
		ids[i] = idStream.str();
		if((i % nodeCount)== nodeId ){
			Logger::debug("Created locally object %d\n", i);
			BankAccount *ba = new BankAccount(10000, ids[i]);
			DirectoryManager::registerObject(*ba, 0);
		}
	}
	return ids;
}

} /* namespace vt_dstm */
