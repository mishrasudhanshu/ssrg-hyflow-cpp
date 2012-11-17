/*
 * HashMapBenchMark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HashMapBenchMark.h"
#include <cstdlib>
#include "../../../util/concurrent/ThreadMeta.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../util/logging/Logger.h"

#define HYFLOW_HASHMAP_CONTENTION 2

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> HashMapBenchmark::objectCreated;

HashMapBenchmark::HashMapBenchmark() { }

HashMapBenchmark::~HashMapBenchmark() {}

int HashMapBenchmark::getOperandsCount()	{
	return 1;
}

void HashMapBenchmark::readOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	if (random%2 == 1 ) {
		LOG_DEBUG("LIST :SUM Nodes\n");
		HashMap::sumNodes();
	}else {
		int value = random%HYFLOW_HASHMAP_CONTENTION;
		LOG_DEBUG("LIST :FIND[%d] Node\n", value);
		HashMap::findNode(value);
	}
}

void HashMapBenchmark::writeOperation(std::string ids[], int size){
	int random = abs(Logger::getCurrentMicroSec());
	int value = random%HYFLOW_HASHMAP_CONTENTION;
	if (random%2 == 1 ) {
		LOG_DEBUG("LIST :ADD[%d] Node\n", value);
		HashMap::addNode(value);
	}else {
		LOG_DEBUG("LIST :DEL[%d] Node\n", value);
		HashMap::deleteNode(value);
	}
}

void HashMapBenchmark::checkSanity(){}

int HashMapBenchmark::getId() {
	HyInteger* objectCount = objectCreated.get();
	if (!objectCount) {
		objectCreated.reset(new HyInteger(0));
		objectCreated.get()->setValue(ThreadMeta::getThreadId()*50000);
	}
	objectCreated->increaseValue();
	return objectCreated.get()->getValue();
}

std::string* HashMapBenchmark::createLocalObjects(int objCount) {
	std::string* ids = NULL;
	ids = new std::string [objCount];
	if (NetworkManager::getNodeId() == 0 ) {
		std::string next("NULL");
		HashMap headNode(0, "HEAD");
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
