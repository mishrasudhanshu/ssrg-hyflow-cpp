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
int ListBenchmark::objectCount = 0 ;

ListBenchmark::ListBenchmark() {}

ListBenchmark::~ListBenchmark() {}

int ListBenchmark::getOperandsCount()	{
	return 1*BenchmarkExecutor::getInnerTxns();
}

void ListBenchmark::warmUp() {
	LOG_DEBUG("***LIST :Warming Up the List Benchmark***\n");
	int nodeCount = NetworkManager::getNodeCount();
	int nodeId = NetworkManager::getNodeId();
	for(int i=0 ; i<(objectCount/2) ; i++){
		if(( i%nodeCount )== nodeId ){
			int value = i;
			LOG_DEBUG("LIST :ADD[%d] Node\n", value);
			ListNode::addNode(value);
		}
	}
}

void ListBenchmark::readOperation(std::string ids[], int size){
	int multiCount = getOperandsCount();
	int* values = new int[multiCount];

	for(int txns = 0; txns<multiCount ; txns++) {
		values[txns] = (abs(Logger::getCurrentMicroSec()+1)+multiCount*txns)%objectCount;
		LOG_DEBUG("LIST :FIND[%d] Node\n", values[txns]);
	}

	ListNode::findNodeMulti(values, multiCount);
	delete[] values;

}

void ListBenchmark::writeOperation(std::string ids[], int size){
	int select = abs(Logger::getCurrentMicroSec()+1);
	int multiCount = getOperandsCount();
	int* values = new int[multiCount];

	if (select%2 == 1 ) {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (abs(Logger::getCurrentMicroSec()+1)+multiCount*txns)%objectCount;
			LOG_DEBUG("LIST :ADD[%d] Node\n", values[txns]);
		}
		ListNode::addNodeMulti(values,multiCount);
	}else {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (abs(Logger::getCurrentMicroSec()+1)+multiCount*txns)%objectCount;
			LOG_DEBUG("LIST :DEL[%d] Node\n", values[txns]);
		}
		ListNode::deleteNodeMulti(values, multiCount);
	}
	delete [] values;
}

void ListBenchmark::checkSanity(){}

int ListBenchmark::getId() {
	HyInteger* objCount = objectCreated.get();
	if (!objCount) {
		objectCreated.reset(new HyInteger(0));
		objectCreated.get()->setValue(ThreadMeta::getThreadId()*50000);
	}
	objectCreated->increaseValue();
	return objectCreated.get()->getValue();
}

std::string* ListBenchmark::createLocalObjects(int objCount) {
	objectCount = objCount;
	int nodeId = NetworkManager::getNodeId();
	ids = new std::string [objCount];

	if (nodeId == 0) {
		ListNode listNode(-1, "HEAD");
		listNode.setNextId("NULL");
		DirectoryManager::registerObject(&listNode, 0);
		LOG_DEBUG("Created List Sentinel Node HEAD with next as NULL\n");
	}

	for(int i=0; i<objCount ; i++){
		ids[i] = "0-0";
	}

	return ids;
}

} /* namespace vt_dstm */
