/*
 * ListBenchmark.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdlib>
#include "ListBenchmark.h"
#include "../../../util/concurrent/ThreadMeta.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../util/logging/Logger.h"

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> ListBenchmark::objectCreated;

ListBenchmark::ListBenchmark() { }

ListBenchmark::~ListBenchmark() {}

int ListBenchmark::getOperandsCount()	{
	return 1;
}

void ListBenchmark::readOperation(std::string ids[], int size){
	if (Logger::getCurrentMicroSec()%2U == 1 ) {
		LOG_DEBUG("LIST :Sum Node\n");
		ListNode::sumNodes();
	}else {
		LOG_DEBUG("LIST :Find Node\n");
		ListNode::findNode(10);
	}
}

void ListBenchmark::writeOperation(std::string ids[], int size){
	if (Logger::getCurrentMicroSec()%2U == 1 ) {
		LOG_DEBUG("LIST :Add Node\n");
		ListNode::addNode(10);
	}else {
		LOG_DEBUG("LIST :Delete Node\n");
		ListNode::deleteNode(10);
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
	std::string* ids = NULL;
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
