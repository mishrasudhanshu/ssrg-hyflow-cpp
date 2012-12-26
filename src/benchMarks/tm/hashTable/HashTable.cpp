/*
 * HashMap.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HashTable.h"

#include <string>
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
//#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../BenchmarkExecutor.h"
#include "HashTableBenchMark.h"

namespace vt_dstm {

HashBucket::HashBucket() {}

HashBucket::HashBucket(std::string id) {
	hyId = id;
	hyVersion = 0;
}

HashBucket::~HashBucket() {
}

bool HashBucket::putInternal(std::pair<int, double> entry) {
	std::vector<int>::iterator bucketItr;
	bool found = false;
	for ( bucketItr = bucketKeys.begin(); bucketItr != bucketKeys.end() ; bucketItr++ ) {
		int currentKey = *bucketItr;
		if ( currentKey == entry.first ) {
			found = true;
			break;
		}
	}

	if (!found) {
		bucketKeys.push_back(entry.first);
		bucketValues.push_back(entry.second);
	}

	return !found;
}

bool HashBucket::removeInternal(int key) {
	std::vector<int>::iterator bucketItr;
	bool found = false;
	int pos=0;
	for ( bucketItr = bucketKeys.begin(); bucketItr != bucketKeys.end() ; bucketItr++ ) {
		int currentKey = *bucketItr;
		if ( currentKey == key ) {
			found = true;
			break;
		}
		pos++;
	}
	if (found) {
		bucketKeys.erase(bucketKeys.begin()+pos);
		bucketValues.erase(bucketValues.begin()+pos);
	}
	return found;
}

std::pair<int, double> HashBucket::getInternal(int key) {
	std::pair<int, double> entry(-1, 0);
	std::vector<int>::iterator bucketItr;
	bool found = false;
	int pos=0;
	for ( bucketItr = bucketKeys.begin(); bucketItr != bucketKeys.end() ; bucketItr++ ) {
		int currentKey = *bucketItr;
		if ( currentKey == key ) {
			found = true;
			break;
		}
		pos++;
	}
	if (found) {
		entry.first = bucketKeys.at(pos);
		entry.second = bucketValues.at(pos);
	}
	return entry;
}

void HashBucket::putAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* pRet) {
	std::pair<int, double> entry = *(((HTArgs*)args)->entries);
	HTReturn *hrt = (HTReturn*) pRet;

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<entry.first;
		std::string lockName = absLockStr.str();
		__context__->onLockAccess("HT0", lockName, false);
	}

	std::string targetBucket = HashTableBenchmark::getBucketId(entry.first);
	HYFLOW_FETCH(targetBucket, false);

	HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
	hrt->success = bucket->putInternal(entry);
	LOG_DEBUG("HashMap :Put in bucket %s value %d\n", bucket->getId().c_str(), entry.first);
}

void HashBucket::removeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rRet) {
	int key = *(((HTArgs*)args)->key1);
	HTReturn *hrt = (HTReturn*) rRet;

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<key;
		std::string lockName = absLockStr.str();
		__context__->onLockAccess("HT0", lockName, false);
	}

	std::string targetBucket = HashTableBenchmark::getBucketId(key);
	HYFLOW_FETCH(targetBucket, false);

	HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
	hrt->success = bucket->removeInternal(key);
	LOG_DEBUG("HashMap :Remove %s in bucket %s value %d\n", hrt->success?"true":"false", bucket->getId().c_str(), key);
}

void HashBucket::getAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* hRet) {
	int key = *(((HTArgs*)args)->key1);
	std::string targetBucket = HashTableBenchmark::getBucketId(key);
	HYFLOW_FETCH(targetBucket, false);

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<key;
		std::string lockName = absLockStr.str();
		__context__->onLockAccess("HT0", lockName, true);
	}

	HashBucket* bucket =  (HashBucket*)HYFLOW_ON_READ(targetBucket);
	((HTReturn*)hRet)->entry = bucket->getInternal(key);
	LOG_DEBUG("HashMap :Get in bucket %s value %d\n", bucket->getId().c_str(), key);
}

bool HashBucket::put(std::pair<int, double> entry) {
	HTArgs hArgs(&entry);
	HTReturn hret;
	Atomic atomicPut;

	atomicPut.atomically = HashBucket::putAtomically;
	atomicPut.onAbort = HashBucket::putAbort;
	atomicPut.execute(NULL, &hArgs, &hret);
	return hret.success;
}

bool HashBucket::remove(int key) {
	HTArgs hArgs(&key, NULL, 1);
	HTReturn hRet;
	Atomic atomicRemove;

	atomicRemove.atomically = HashBucket::removeAtomically;
	atomicRemove.onAbort = HashBucket::removeAbort;
	atomicRemove.execute(NULL, &hArgs, &hRet);
	return hRet.success;
}

std::pair<int, double> HashBucket::get(int key) {
	HTReturn hRet;
	HTArgs hArgs(&key, NULL, 1);
	Atomic atomicGet;

	atomicGet.atomically = HashBucket::getAtomically;
	atomicGet.execute(NULL, &hArgs, &hRet);
	return hRet.entry;
}

void HashBucket::putMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore) {
	HTArgs* htArgs = (HTArgs*)args;
	for (int txns = 0; txns < htArgs->size ; txns+=1) {
		put(htArgs->entries[txns]);
	}
}
void HashBucket::removeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore) {
	HTArgs* htArgs = (HTArgs*)args;
	for (int txns = 0; txns < htArgs->size ; txns+=1) {
		remove(htArgs->key1[txns]);
	}
}

void HashBucket::getMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* entry) {
	HTArgs* htArgs = (HTArgs*)args;
	for (int txns = 0; txns < htArgs->size ; txns+=1) {
		get(htArgs->key1[txns]);
	}
}

void HashBucket::putMulti(std::pair<int, double> entry[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("HT :Call Put Node with %d in txns %d\n", entry[txns].first, txns);
				put(entry[txns]);

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs htArgs(NULL, NULL, size);
		htArgs.entries = entry;
		Atomic atomicPutMulti;
		atomicPutMulti.atomically = HashBucket::putMultiAtomically;
		// Don't need to assign onAbort & onCommit as nothing specific required
		// lock handling is automatically done by context level, not in onAbort etc.
		atomicPutMulti.execute(NULL, &htArgs, NULL);
	}
}

void HashBucket::removeMulti(int values[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("HT :Call removes Node with %d in txns %d\n", values[txns], txns);
				remove(values[txns]);

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs htArgs(values, NULL, size);
		Atomic atomicRemoveMulti;
		atomicRemoveMulti.atomically = HashBucket::removeMultiAtomically;
		atomicRemoveMulti.execute(NULL, &htArgs, NULL);
	}
}

void HashBucket::getMulti(int values[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("HT :Call Get Node with %d in txns %d\n", values[txns], txns);
				get(values[txns]);

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs htArgs(values, NULL, size);
		Atomic atomicGetMulti;
		atomicGetMulti.atomically = HashBucket::getMultiAtomically;
		atomicGetMulti.execute(NULL, &htArgs, NULL);
	}
}

void HashBucket::print() {}

void HashBucket::getClone(HyflowObject **obj) {
	HashBucket *ln = new HashBucket();
	ln->bucketKeys = bucketKeys;
	ln->bucketValues = bucketValues;
	this->baseClone(ln);
	*obj = ln;
}

void HashBucket::test() {
	// create and open a character archive for output
	std::ofstream ofs("HashBucket", std::ios::out);

	// create class instance
	std::string id="0-0";
	vt_dstm::HashBucket  l;
	l.setId(id);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << l;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	vt_dstm::HashBucket l1;
	{
		// create and open an archive for input
		std::ifstream ifs("HashBucket", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> l1;
		// archive and stream closed when destructors are called
		l1.print();
	}
}

void HashBucket::putAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	HTArgs *hargs = (HTArgs*) args;
	HTReturn* hrt = (HTReturn*) rt;
	if (hrt->success) {
		int key = hargs->entries[0].first;

		std::string targetBucket = HashTableBenchmark::getBucketId(key);
		HYFLOW_FETCH(targetBucket, false);

		HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
		bool result = bucket->removeInternal(key);
		if (!result) {
			// Sort of sanity check, if it fails issue in implementation
			Logger::fatal("HashMap :Rollback of put = remove %d operation failed\n", key);
		}
		LOG_DEBUG("HashMap :Remove %s in bucket %s value %d\n", hrt->success?"true":"false", bucket->getId().c_str(), key);
	}
}

void HashBucket::removeAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	HTArgs *hargs = (HTArgs*) args;
	HTReturn* hrt = (HTReturn*) rt;
	if (hrt->success) {
		//TODO: Return the value instead of success, it can be used to create entry
		std::pair<int, double> entry(*(hargs->key1),0.1);

		std::string targetBucket = HashTableBenchmark::getBucketId(entry.first);
		HYFLOW_FETCH(targetBucket, false);

		HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
		bool result = bucket->putInternal(entry);
		if (!result) {
			// Sort of sanity check, if it fails issue in implementation
			Logger::fatal("HashMap :Rollback of remove = put %d operation failed\n", entry.first);
		}
		LOG_DEBUG("HashMap :Put in bucket %s value %d\n", bucket->getId().c_str(), entry.first);
	}
}

} /* namespace vt_dstm */
