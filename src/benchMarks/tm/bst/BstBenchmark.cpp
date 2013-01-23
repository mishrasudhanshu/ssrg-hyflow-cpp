/*
 * BstBenchmark.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: mishras[at]vt.edu
 */

#include "BstBenchmark.h"
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

boost::thread_specific_ptr<HyInteger> BstBenchmark::objectCreated;

BstBenchmark::BstBenchmark() { objectCount=0; }

BstBenchmark::~BstBenchmark() {}

int BstBenchmark::getOperandsCount()	{
	return 1*BenchmarkExecutor::getInnerTxns();
}

void BstBenchmark::warmUp() {
	if (BenchmarkExecutor::isDoWarmUp()) {
		LOG_DEBUG("***BST :Warming Up the Bst Benchmark***\n");
		int nodeCount = NetworkManager::getNodeCount();
		int nodeId = NetworkManager::getNodeId();
		std::vector<int> sequence = bstSequence(0, objectCount, 2);
		for(unsigned int i=0 ; i<sequence.size() ; i++){
			if(( i%nodeCount )== nodeId ){
				for (int bstN = 1; bstN <= HYFLOW_SKIPLIST_COUNT ; bstN++) {
					int value = sequence.at(i);
					LOG_DEBUG("BST :ADD[%d] Node\n", value);
					sleep(nodeId);
					BstNode::addNode(value);
					sleep(nodeCount-nodeId);
				}
			}
		}
	}
}

std::vector<int> BstBenchmark::bstSequence(int start, int end, int diff) {
	std::vector<int> linearSeq;
	for (int itr=start ; itr < end ; itr+=diff) {
		linearSeq.push_back(itr);
	}

	int* locationArray = new int[linearSeq.size()];
	sequenceUtil(locationArray, 0, linearSeq.size()-1, 0, linearSeq.size());

	std::vector<int> treeSequence;
	for (unsigned int tItr=0 ; tItr < linearSeq.size(); tItr++ ) {
		treeSequence.push_back(linearSeq.at(locationArray[tItr]));
	}
	delete [] locationArray;
	return treeSequence;
}

void BstBenchmark::sequenceUtil(int seq[], int start, int end, int pos, int size) {
	if ( pos < size ) {
		int mid = (start + end + 1)/2;
		seq[pos] = mid;
		sequenceUtil(seq, start, (2*mid-1)/2, 2*pos+1, size);
		sequenceUtil(seq, (2*mid+1)/2, end, 2*pos+2, size);
	}
}

void BstBenchmark::readOperation(std::string ids[], int size){
	int multiCount = getOperandsCount();
	int* values = new int[multiCount];

	for(int txns = 0; txns<multiCount ; txns++) {
		values[txns] = (abs(Logger::getCurrentMicroSec()+1)+multiCount*txns)%objectCount;
		LOG_DEBUG("BST :FIND[%d] Node\n", values[txns]);
	}

	BstNode::findNodeMulti(values, multiCount);
	delete[] values;
}

void BstBenchmark::writeOperation(std::string ids[], int size){
	int select = abs(Logger::getCurrentMicroSec()+1);
	int multiCount = getOperandsCount();
	int* values = new int[multiCount];

	if (select%2 == 1 ) {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (abs(Logger::getCurrentMicroSec()+1)+multiCount*txns)%objectCount;
			LOG_DEBUG("BST :ADD[%d] Node\n", values[txns]);
		}
		BstNode::addNodeMulti(values,multiCount);
	}else {
		for(int txns = 0; txns<multiCount ; txns++) {
			values[txns] = (abs(Logger::getCurrentMicroSec()+1)+multiCount*txns)%objectCount;
			LOG_DEBUG("BST :DEL[%d] Node\n", values[txns]);
		}
		BstNode::deleteNodeMulti(values, multiCount);
	}
	delete [] values;
}

void BstBenchmark::checkSanity(){}

int BstBenchmark::getId() {
	HyInteger* objectCount = objectCreated.get();
	if (!objectCount) {
		objectCreated.reset(new HyInteger(0));
		objectCreated.get()->setValue(ThreadMeta::getThreadId()*50000);
	}
	objectCreated->increaseValue();
	return objectCreated.get()->getValue();
}

std::string* BstBenchmark::createLocalObjects(int objCount) {
	objectCount = objCount;
	ids = new std::string [objCount];
	if (NetworkManager::getNodeId() == 0 ) {
		for (int bstN = 1; bstN <= HYFLOW_SKIPLIST_COUNT ; bstN++) {
			std::stringstream headStr;
			headStr<<"HEAD-"<<bstN;
			std::string head = headStr.str();
			std::string next("NULL");
			BstNode headNode(0, head, bstN);
			DirectoryManager::registerObject(&headNode, 0);
		}
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
