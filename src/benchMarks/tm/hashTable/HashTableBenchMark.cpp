/*
 * HashMapBenchMark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HashTableBenchMark.h"
#include <cstdlib>
#include "../../../util/concurrent/ThreadMeta.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../util/logging/Logger.h"

#define HYFLOW_HASHTABLE_CONTENTION 2

namespace vt_dstm {
int HashTableBenchmark::objectCount = 0;
int HashTableBenchmark::bucketCount = 0;

boost::thread_specific_ptr<HyInteger> HashTableBenchmark::objectCreated;

HashTableBenchmark::HashTableBenchmark() { }

HashTableBenchmark::~HashTableBenchmark() {}

int HashTableBenchmark::getOperandsCount()	{
	return 2;
}

void HashTableBenchmark::readOperation(std::string ids[], int size){
	int key = abs(Logger::getCurrentMicroSec()%objectCount);
	LOG_DEBUG("HashTable :GET[%d] Nodes\n", key);
	HashBucket::get(key);
}

void HashTableBenchmark::writeOperation(std::string ids[], int size){
	int key = abs(Logger::getCurrentMicroSec()%objectCount);
	if (key%3 == 0 ) {
		LOG_DEBUG("HashTable :PUT[%d] Entry\n", key);
		std::pair<int, double> entry(key, 0.1);
		HashBucket::put(entry);
	}else if(key%3 == 1){
		LOG_DEBUG("HashTable :DEL[%d] Entry\n", key);
		HashBucket::remove(key);
	}else {
		int key2 = abs(Logger::getCurrentMicroSec()%objectCount);
		LOG_DEBUG("HashTable :MV[%d][%d] Entry\n", key, key2);
		HashBucket::move(key, key2);
	}
}

void HashTableBenchmark::checkSanity(){}

std::string HashTableBenchmark::getBucketId(int key) {
	int keyBucket = key%bucketCount;
	int keyNode = keyBucket%NetworkManager::getNodeCount();
	std::stringstream idStr;
	idStr<<keyNode<<"-"<<keyBucket;
	return idStr.str();
}

std::string* HashTableBenchmark::createLocalObjects(int objCount) {
	ids = new std::string [objCount];
	objectCount = objCount;
	int nodeCount =  NetworkManager::getNodeCount();
	int nodeId = NetworkManager::getNodeId();
	bucketCount = (int)objCount/HYFLOW_HASHTABLE_CONTENTION;
	for(int i=0; i<bucketCount ; i++){
		std::ostringstream idStream;
		idStream << i%nodeCount <<"-"<< i;
		if((i % nodeCount)== nodeId ){
			LOG_DEBUG("Created locally object %d\n", i);
			// Create a stack copy of object
			// register account will create a heap copy of object
			// and save its pointer in local cache
			HashBucket hb(idStream.str());
			DirectoryManager::registerObjectWait(&hb, 0);
		}
	}

	for (int i = 0; i < objCount; i++) {
		std::ostringstream idStream;
		idStream << 0 << "-" << 0;
		ids[i] = idStream.str();
	}
	return ids;
}

} /* namespace vt_dstm */
