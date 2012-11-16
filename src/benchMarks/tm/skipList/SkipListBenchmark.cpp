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

// TODO: Move to default configuration
#define HYFLOW_SKIP_LIST_CONTENTION 3
#define HYFLOW_SKIP_LIST_LEVELS 5

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> SkipListBenchmark::objectCreated;
int SkipListBenchmark::skipListLevels=HYFLOW_SKIP_LIST_LEVELS;

SkipListBenchmark::SkipListBenchmark() { }

SkipListBenchmark::~SkipListBenchmark() {}

int SkipListBenchmark::getOperandsCount()	{
	return 1;
}

void SkipListBenchmark::readOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	int value = random%HYFLOW_SKIP_LIST_CONTENTION;
	LOG_DEBUG("LIST :FIND[%d] Node\n", value);
	SkipListNode::findNode(value);
}

void SkipListBenchmark::writeOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	int value = random%HYFLOW_SKIP_LIST_CONTENTION;
	if (random%2 == 1 ) {
		LOG_DEBUG("LIST :ADD[%d] Node\n", value);
		SkipListNode::addNode(value);
	}else {
		LOG_DEBUG("LIST :DEL[%d] Node\n", value);
		SkipListNode::deleteNode(value);
	}
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
	std::string* ids = NULL;
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
