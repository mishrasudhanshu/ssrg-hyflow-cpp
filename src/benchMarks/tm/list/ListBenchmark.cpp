/*
 * ListBenchmark.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#include "ListBenchmark.h"
#include <cstdlib>
#include "../../../util/concurrent/ThreadMeta.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../util/logging/Logger.h"
#include "../../BenchmarkExecutor.h"

/*
 * Increasing object count causes the longer lists and more contention
 * Higher object count causes the deletion of object less probable
 */

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> ListBenchmark::objectCreated;

ListBenchmark::ListBenchmark() {
	objectCount = 0;
}

ListBenchmark::~ListBenchmark() {}

int ListBenchmark::getOperandsCount()	{
	return 1*BenchmarkExecutor::getInnerTxns();
}

void ListBenchmark::readOperation(std::string ids[], int size){
	int multiCount = getOperandsCount();
	int select = abs(Logger::getCurrentMicroSec()+1);
	if (select%2 == 1 ) {
		LOG_DEBUG("LIST :SUM Nodes\n");
		ListNode::sumNodesMulti(multiCount);
	}else {
		int random = abs(Logger::getCurrentMicroSec());
		int* values = new int[multiCount];

		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (random+txns)%objectCount;
			LOG_DEBUG("LIST :FIND[%d] Node\n", values[txns]);
		}

		ListNode::findNodeMulti(values, multiCount);
		delete[] values;
	}
}

void ListBenchmark::writeOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	int select = abs(Logger::getCurrentMicroSec()+1);
	int multiCount = getOperandsCount();
	int* values = new int[multiCount];

	if (select%2 == 1 ) {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (random+multiCount)%objectCount;
			LOG_DEBUG("LIST :ADD[%d] Node\n", values[txns]);
		}
		ListNode::addNodeMulti(values,multiCount);
	}else {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (random+multiCount)%objectCount;
			LOG_DEBUG("LIST :DEL[%d] Node\n", values[txns]);
		}
		ListNode::deleteNodeMulti(values, multiCount);
	}
	delete [] values;
}

void ListBenchmark::checkSanity(){}

int ListBenchmark::getId() {
	HyInteger* objectCount = objectCreated.get();
	if (!objectCount) {
		objectCreated.reset(new HyInteger(0));
		objectCreated.get()->setValue(ThreadMeta::getThreadId()*50000);
	}
	objectCreated->increaseValue();
	return objectCreated.get()->getValue();
}

std::string* ListBenchmark::createLocalObjects(int objCount) {
	objectCount = objCount;
	ids = new std::string [objCount];
	if (NetworkManager::getNodeId() == 0 ) {
		std::string next("NULL");
		ListNode headNode(0, "HEAD");
		headNode.setNextId(next);
		DirectoryManager::registerObject(&headNode, 0);
	}
	// TODO : Don't Provide Random Ids, we don't need
	for (int i = 0; i < objCount; i++) {
		std::ostringstream idStream;
		idStream << 0 << "-" << 0;
		ids[i] = idStream.str();
	}
	return ids;
}

} /* namespace vt_dstm */
