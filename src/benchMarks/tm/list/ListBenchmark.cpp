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

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> ListBenchmark::objectCreated;

ListBenchmark::ListBenchmark() { }

ListBenchmark::~ListBenchmark() {}

int ListBenchmark::getOperandsCount()	{
	return 1;
}

void ListBenchmark::readOperation(std::string ids[], int size){
	if (rand()%2 == 1 ) {
		ListNode::sumNodes();
	}else {
		ListNode::findNode(10);
	}
}

void ListBenchmark::writeOperation(std::string ids[], int size){
	if (rand()%2 == 1 ) {
		ListNode::addNode(10);
	}else {
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
	if (NetworkManager::getNodeId() == 0 ) {
		std::string next("NULL");
		ListNode headNode(0, "0-0");
		headNode.setNextId(next);
		DirectoryManager::registerObject(&headNode, 0);
	}
	return NULL;
}

} /* namespace vt_dstm */
