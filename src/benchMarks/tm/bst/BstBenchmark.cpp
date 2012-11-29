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

/*
 * Increasing contention level causes the longer lists and more contention
 * Higher contention value causes the deletion of object less probable
 * TODO: Set as configurable value from default.conf
 */
#define HYFLOW_BST_CONTENTION 3		//Should create around 5 nodes

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> BstBenchmark::objectCreated;

BstBenchmark::BstBenchmark() { }

BstBenchmark::~BstBenchmark() {}

int BstBenchmark::getOperandsCount()	{
	return 1;
}

void BstBenchmark::readOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());

	int value = random%HYFLOW_BST_CONTENTION;
	LOG_DEBUG("BST :FIND[%d] Node\n", value);
	BstNode::findNode(value);

}

void BstBenchmark::writeOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	int value = random%HYFLOW_BST_CONTENTION;
	int select = abs(Logger::getCurrentMicroSec()+1)%2;
	if (select) {
		LOG_DEBUG("BST :ADD[%d] Node\n", value);
		BstNode::addNode(value);
	}else {
		LOG_DEBUG("BST :DEL[%d] Node\n", value);
		BstNode::deleteNode(value);
	}
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
	ids = new std::string [objCount];
	if (NetworkManager::getNodeId() == 0 ) {
		std::string next("NULL");
		BstNode headNode(0, "HEAD");
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
