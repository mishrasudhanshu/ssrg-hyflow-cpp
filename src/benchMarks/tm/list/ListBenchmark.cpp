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

/*
 * Increasing contention level causes the longer lists and more contention
 * Higher contention value causes the deletion of object less probable
 * TODO: Set as configurable value from default.conf
 */
#define HYFLOW_LIST_CONTENTION 2

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> ListBenchmark::objectCreated;

ListBenchmark::ListBenchmark() { }

ListBenchmark::~ListBenchmark() {}

int ListBenchmark::getOperandsCount()	{
	return 1;
}

void ListBenchmark::readOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	if (random%2 == 1 ) {
		LOG_DEBUG("LIST :SUM Nodes\n");
		ListNode::sumNodes();
	}else {
		int value = random%HYFLOW_LIST_CONTENTION;
		LOG_DEBUG("LIST :FIND[%d] Node\n", value);
		ListNode::findNode(value);
	}
}

void ListBenchmark::writeOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	int value = random%HYFLOW_LIST_CONTENTION;
	int select = abs(Logger::getCurrentMicroSec()+1);
	if (select%2 == 1 ) {
		LOG_DEBUG("LIST :ADD[%d] Node\n", value);
		ListNode::addNode(value);
	}else {
		LOG_DEBUG("LIST :DEL[%d] Node\n", value);
		ListNode::deleteNode(value);
	}
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
