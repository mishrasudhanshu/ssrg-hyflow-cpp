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
	}

	HTArgs(std::pair<int, double> *entry) {
		key1 = NULL;
		key2 = NULL;
		size = 1;
		entries = entry;
	}

	void getClone(BenchMarkArgs **args) {

	}
};

class HTReturn: public BenchMarkReturn {
public:
	std::pair<int, double> entry;

	void getClone(BenchMarkReturn **benchArgs) {

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
	void putInternal(std::pair<int, double> entry);
	void removeInternal(int key);
	std::pair<int, double> getInternal(int key);

	static void putAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void removeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void moveAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void getAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* pair);

	static void putMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void removeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void moveMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);
	static void getMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore);

	static void put(std::pair<int, double> entry);
	static void remove(int value);
	static void move(int key1, int key2);
	static std::pair<int, double> get(int value);
public:
	HashBucket();
	HashBucket(std::string id);
	virtual ~HashBucket();

	void print();
	void getClone(HyflowObject **obj);

	static void getMultiAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);
	static void putMultiAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);
	static void removeMultiAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);
	static void moveMultiAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);

	static void getMultiCommit(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);
	static void putMultiCommit(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);
	static void removeMultiCommit(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);
	static void moveMultiCommit(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__);

	static void putMulti(std::pair<int, double> entry[], int size);
	static void removeMulti(int values[], int size);
	static void moveMulti(int key1[], int key2[], int size);
	static void getMulti(int values[], int size);

	void test();
};

} /* namespace vt_dstm */

#endif /* HASHTABLE_H_ */
