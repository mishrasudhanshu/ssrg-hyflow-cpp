/*
 * HashMap.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/helper/BenchMarkArgs.h"
#include "../../../core/helper/BenchMarkReturn.h"
#include "../../../util/logging/Logger.h"

namespace vt_dstm {

class HTArgs: public BenchMarkArgs{
public:
	int* key1;
	int* key2;
	std::pair<int, double> *entries;

	int size;
	HTArgs() {}
	HTArgs(int *k1, int *k2, int s) {
		key1 = k1;
		key2 = k2;
		size = s;
		entries = NULL;
		onHeap = false;
	}

	HTArgs(std::pair<int, double> *entry) {
		key1 = NULL;
		key2 = NULL;
		size = 1;
		entries = entry;
		onHeap = false;
	}

	~HTArgs() {
		if (onHeap) {
			delete key1;
			delete key2;
			delete entries;
		}
	}

	void getClone(BenchMarkArgs **args) {
		int *k1= NULL, *k2=NULL;
		std::pair<int, double> *ents = NULL;
		if (key1) {
			k1 = new int[size];
		}
		if (key2) {
			k2 = new int[size];
		}
		if (entries) {
			ents = new std::pair<int, double>[size];
		}
		for (int i=0 ; i<size ; i++ ) {
			if (key1) {
				k1[i] = key1[i];
			}
			if (key2) {
				k2[i] = key2[i];
			}
			if (entries) {
				ents[i] = entries[i];
			}
		}
		HTArgs* clone = new HTArgs(k1, k2, size);
		clone->key1 = k1;
		clone->key2 = k2;
		clone->entries = ents;
		clone->onHeap = true;
		LOG_DEBUG("HTB :Copied the arguments as key1 %p, key2 %p and entries %p\n", clone->key1, clone->key2, clone->entries);
		*args = clone;
	}
};

class HTReturn: public BenchMarkReturn {
public:
	std::pair<int, double> entry;
	bool success;

	HTReturn() {
		success = false;
		onHeap = false;
	}

	void getClone(BenchMarkReturn **benchArgs) {
		HTReturn* hrt = new HTReturn();
		hrt->entry = entry;
		hrt->success = success;
		hrt->onHeap = true;
		*benchArgs = hrt;
	}
};

class HashBucket: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & bucketKeys;
    	ar & bucketValues;
    }

	std::vector<int > bucketKeys;
	std::vector<double> bucketValues;
	/*
	 * Adds the given entry in bucket if key does not exist, if already exist do nothing
	 * Returns success, if new entry is added in the hashTable
	 */
	bool putInternal(std::pair<int, double> entry);
	/*
	 * Return success, if key already exist in bucket and removed successfully
	 */
	bool removeInternal(int key);
	std::pair<int, double> getInternal(int key);

	static void putAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void removeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void getAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* pair);

	static void putMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void removeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void getMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);

	static bool put(std::pair<int, double> entry);
	static bool remove(int value);
	static std::pair<int, double> get(int value);
public:
	HashBucket();
	HashBucket(std::string id);
	virtual ~HashBucket();

	void print();
	void getClone(HyflowObject **obj);

	static void putAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);
	static void removeAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);

	static void putMulti(std::pair<int, double> entry[], int size);
	static void removeMulti(int values[], int size);
	static void getMulti(int values[], int size);

	void test();
};

} /* namespace vt_dstm */

#endif /* HASHTABLE_H_ */
