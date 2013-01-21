/*
 * SkipListBenchmark.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: mishras[at]vt.edu
 */

#include "SkipListBenchmark.h"
#include <cstdlib>
#include "SkipListBenchmark.h"
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

boost::thread_specific_ptr<HyInteger> SkipListBenchmark::objectCreated;
int SkipListBenchmark::skipListLevels=5;

SkipListBenchmark::SkipListBenchmark() { objectCount=0; }

SkipListBenchmark::~SkipListBenchmark() {}

int SkipListBenchmark::getOperandsCount()	{
	return 1*BenchmarkExecutor::getInnerTxns();
}

void SkipListBenchmark::warmUp() {
	LOG_DEBUG("***SKIPLIST :Warming Up the SKIPList Benchmark***\n");
	int nodeCount = NetworkManager::getNodeCount();
	int nodeId = NetworkManager::getNodeId();
	for(int i=0 ; i<(objectCount/2) ; i++){
		if(( i%nodeCount )== nodeId ){
			int value = i;
			LOG_DEBUG("SKIPLIST :ADD[%d] Node\n", value);
			sleep(nodeId);
			SkipListNode::addNode(value);
			sleep(nodeCount-nodeId);
		}
	}
}

void SkipListBenchmark::readOperation(std::string ids[], int size){
	int multiCount = getOperandsCount();

	int* values = new int[multiCount];
	for(int txns = 0; txns<multiCount ; txns++) {
		values[txns] = (abs(Logger::getCurrentMicroSec())+txns*multiCount)%objectCount;
		LOG_DEBUG("SkipLIST :FIND[%d] Node\n", values[txns]);
	}
	SkipListNode::findNodeMulti(values, multiCount);
	delete[] values;
}

void SkipListBenchmark::writeOperation(std::string ids[], int size){
	int select = abs(Logger::getCurrentMicroSec()+1);
	int multiCount = getOperandsCount();

	int* values = new int[multiCount];
	if (select%2 == 1 ) {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (abs(Logger::getCurrentMicroSec())+multiCount*txns)%objectCount;
			LOG_DEBUG("SkipLIST :ADD[%d] Node\n", values[txns]);
		}
		SkipListNode::addNodeMulti(values,multiCount);
	}else {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (abs(Logger::getCurrentMicroSec())+multiCount*txns)%objectCount;
			LOG_DEBUG("SkipLIST :DEL[%d] Node\n", values[txns]);
		}
		SkipListNode::deleteNodeMulti(values, multiCount);
	}
	delete[] values;
}

void SkipListBenchmark::checkSanity(){}

int SkipListBenchmark::getId() {
	HyInteger* objectCount = objectCreated.get();
	if (!objectCount) {
		objectCreated.reset(new HyInteger(0));
		objectCreated.get()->setValue(ThreadMeta::getThreadId()*50000);
	}
	objectCreated->increaseValue();
	return objectCreated.get()->getValue();
}

std::string* SkipListBenchmark::createLocalObjects(int objCount) {
	objectCount = objCount;
	skipListLevels = BenchmarkExecutor::getObjectNesting();
	ids = new std::string [objCount];
	if (NetworkManager::getNodeId() == 0 ) {
		SkipListNode headNode(0, "HEAD", skipListLevels);
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
