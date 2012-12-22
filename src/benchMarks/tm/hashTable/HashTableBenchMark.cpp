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
#include "../../BenchmarkExecutor.h"

// Defines the number of elements per bucket
#define HYFLOW_HASHTABLE_CONTENTION 2

namespace vt_dstm {
int HashTableBenchmark::objectCount = 0;
int HashTableBenchmark::bucketCount = 0;

boost::thread_specific_ptr<HyInteger> HashTableBenchmark::objectCreated;

HashTableBenchmark::HashTableBenchmark() { }

HashTableBenchmark::~HashTableBenchmark() {}

int HashTableBenchmark::getOperandsCount()	{
	return 1*BenchmarkExecutor::getInnerTxns();
}

void HashTableBenchmark::readOperation(std::string ids[], int size){
	int txnsCount = getOperandsCount();
	int *keys = new int[txnsCount];
	for ( int txns=0 ; txns<txnsCount ; txns++) {
		// Make access to random buckets
		keys[txns] = (abs(Logger::getCurrentMicroSec())+txns)%objectCount;
		LOG_DEBUG("HashTable :GET[%d] Nodes\n", keys[txns]);
	}
	HashBucket::getMulti(keys, txnsCount);
	delete[] keys;
}

void HashTableBenchmark::writeOperation(std::string ids[], int size){
	int select = abs(Logger::getCurrentMicroSec());
	int txnsCount = getOperandsCount();
	if (select%2 == 0 ) {
		std::pair<int, double> *entries = new std::pair<int, double>[txnsCount];
		for ( int txns=0 ; txns<txnsCount ; txns++) {
			// Make access to random buckets
			int key = (abs(Logger::getCurrentMicroSec())+txnsCount)%objectCount;
			entries[txns].first = key;
			entries[txns].second = 0.1;
			LOG_DEBUG("HashTable :PUT[%d] Entry\n", entries[txns].first);
		}
		HashBucket::putMulti(entries, txnsCount);
		delete[] entries;
	}else {
		int *keys = new int[txnsCount];
		for ( int txns=0 ; txns<txnsCount ; txns++) {
			// Make access to random buckets
			keys[txns] = (abs(Logger::getCurrentMicroSec())+txnsCount)%objectCount;
			LOG_DEBUG("HashTable :DEL[%d] Nodes\n", keys[txns]);
		}
		HashBucket::removeMulti(keys, txnsCount);
		delete[] keys;
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
