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
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "HashTableBenchMark.h"

namespace vt_dstm {

HashBucket::HashBucket() {}

HashBucket::HashBucket(std::string id) {
	hyId = id;
	hyVersion = 0;
}

HashBucket::~HashBucket() {
}

void HashBucket::putInternal(std::pair<int, double> entry) {
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
}

void HashBucket::removeInternal(int key) {
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

void HashBucket::putAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	std::pair<int, double> entry = *((std::pair<int, double>*)args);

	std::string targetBucket = HashTableBenchmark::getBucketId(entry.first);
	HYFLOW_FETCH(targetBucket, false);

	HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
	bucket->putInternal(entry);
	LOG_DEBUG("HashMap :Put in bucket %s value %d\n", bucket->getId().c_str(), entry.first);
}

void HashBucket::removeAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	int key = *((int*)args);

	std::string targetBucket = HashTableBenchmark::getBucketId(key);
	HYFLOW_FETCH(targetBucket, false);

	HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
	bucket->removeInternal(key);
	LOG_DEBUG("HashMap :Remove in bucket %s value %d\n", bucket->getId().c_str(), key);
}
void HashBucket::moveAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	HTArgs* htArgs = (HTArgs*)args;

	std::string currentBucket = HashTableBenchmark::getBucketId(htArgs->key1[0]);
	std::string targetBucket = HashTableBenchmark::getBucketId(htArgs->key2[0]);
	HYFLOW_FETCH(currentBucket, false);
	HYFLOW_FETCH(targetBucket, false);

	HashBucket* fromBucket = (HashBucket*)HYFLOW_ON_WRITE(currentBucket);
	HashBucket* toBucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
	std::pair<int ,double> entry = fromBucket->getInternal(htArgs->key1[0]);
	if (entry.first != -1) {
		fromBucket->removeInternal(htArgs->key1[0]);
		toBucket->putInternal(entry);
	}
	LOG_DEBUG("HashMap :Move from %s to %s\n", fromBucket->getId().c_str(), toBucket->getId().c_str());
}
void HashBucket::getAtomically(HyflowObject* self, void* args, HyflowContext* __context__, std::pair<int, double>* entry) {
	int key = *((int*)args);
	std::string targetBucket = HashTableBenchmark::getBucketId(key);
	HYFLOW_FETCH(targetBucket, false);

	HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
	*entry = bucket->getInternal(key);
	LOG_DEBUG("HashMap :Get in bucket %s value %d\n", bucket->getId().c_str(), entry->first);
}

void HashBucket::put(std::pair<int, double> entry) {
	Atomic<void> atomicPut;
	atomicPut.atomically = HashBucket::putAtomically;
	atomicPut.execute(NULL, &entry, NULL);
}

void HashBucket::remove(int key) {
	Atomic<void> atomicPut;
	atomicPut.atomically = HashBucket::removeAtomically;
	atomicPut.execute(NULL, &key, NULL);
}

void HashBucket::move(int key1, int key2) {
	HTArgs htArgs(&key1,&key2,0);
	Atomic<void> atomicPut;
	atomicPut.atomically = HashBucket::moveAtomically;
	atomicPut.execute(NULL, &htArgs, NULL);
}

std::pair<int, double> HashBucket::get(int key) {
	std::pair<int, double> entry;
	Atomic<std::pair<int, double> > atomicPut;
	atomicPut.atomically = HashBucket::getAtomically;
	atomicPut.execute(NULL, &key, &entry);
	return entry;
}

void HashBucket::putMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	HTArgs* htArgs = (HTArgs*)args;
	for (int txns = 0; txns < htArgs->size ; txns+=1) {
		put(htArgs->entries[txns]);
	}
}
void HashBucket::removeMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	HTArgs* htArgs = (HTArgs*)args;
	for (int txns = 0; txns < htArgs->size ; txns+=1) {
		remove(htArgs->key1[txns]);
	}
}
void HashBucket::moveMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	HTArgs* htArgs = (HTArgs*)args;
	for (int txns = 0; txns < htArgs->size ; txns+=1) {
		move(htArgs->key1[txns], htArgs->key2[txns]);
	}
}
void HashBucket::getMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, std::pair<int, double>* entry) {
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

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs htArgs(NULL, NULL, size);
		htArgs.entries = entry;
		Atomic<void> atomicPutMulti;
		atomicPutMulti.atomically = HashBucket::putMultiAtomically;
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

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs htArgs(values, NULL, size);
		Atomic<void> atomicRemoveMulti;
		atomicRemoveMulti.atomically = HashBucket::removeMultiAtomically;
		atomicRemoveMulti.execute(NULL, &htArgs, NULL);
	}
}
void HashBucket::moveMulti(int key1[], int key2[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("HT :Call Get Node with %d in txns %d\n", key1[txns], txns);
				move(key1[txns], key2[txns]);

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs keys(key1, key2, size);
		Atomic<void> atomicMoveMulti;
		atomicMoveMulti.atomically = HashBucket::moveMultiAtomically;
		atomicMoveMulti.execute(NULL, &keys, NULL);
	}
}
void HashBucket::getMulti(int values[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("HT :Call Get Node with %d in txns %d\n", values[txns], txns);
				get(values[txns]);

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		HTArgs htArgs(values, NULL, size);
		Atomic<std::pair<int, double> > atomicGetMulti;
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

	// ... some time later restore the class instance to its orginal state
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


} /* namespace vt_dstm */
