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

void HashBucket::put(std::pair<int, double> entry) {
	HYFLOW_ATOMIC_START{
		std::string targetBucket = HashTableBenchmark::getBucketId(entry.first);
		HYFLOW_FETCH(targetBucket, false);

		HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
		bucket->putInternal(entry);
		LOG_DEBUG("HashMap :Put in bucket %s value %d\n", bucket->getId().c_str(), entry.first);
	} HYFLOW_ATOMIC_END;
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

void HashBucket::remove(int key) {
	HYFLOW_ATOMIC_START{
		std::string targetBucket = HashTableBenchmark::getBucketId(key);
		HYFLOW_FETCH(targetBucket, false);

		HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
		bucket->removeInternal(key);
		LOG_DEBUG("HashMap :Remove in bucket %s value %d\n", bucket->getId().c_str(), key);
	} HYFLOW_ATOMIC_END;
}

void HashBucket::move(int key1, int key2) {
	HYFLOW_ATOMIC_START{
		std::string currentBucket = HashTableBenchmark::getBucketId(key1);
		std::string targetBucket = HashTableBenchmark::getBucketId(key2);
		HYFLOW_FETCH(currentBucket, false);
		HYFLOW_FETCH(targetBucket, false);

		HashBucket* fromBucket = (HashBucket*)HYFLOW_ON_WRITE(currentBucket);
		HashBucket* toBucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
		std::pair<int ,double> entry = fromBucket->getInternal(key1);
		if (entry.first != -1) {
			fromBucket->removeInternal(key1);
			toBucket->putInternal(entry);
		}
		LOG_DEBUG("HashMap :Move from %s to %s\n", fromBucket->getId().c_str(), toBucket->getId().c_str());
	} HYFLOW_ATOMIC_END;
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

std::pair<int, double> HashBucket::get(int key) {
	std::pair<int, double> entry;
	HYFLOW_ATOMIC_START{
		std::string targetBucket = HashTableBenchmark::getBucketId(key);
		HYFLOW_FETCH(targetBucket, false);

		HashBucket* bucket =  (HashBucket*)HYFLOW_ON_WRITE(targetBucket);
		bucket->getInternal(key);
		LOG_DEBUG("HashMap :Get in bucket %s value %d\n", bucket->getId().c_str(), entry.first);
	} HYFLOW_ATOMIC_END;
	return entry;
}

void HashBucket::print() {

}

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
