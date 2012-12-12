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

namespace vt_dstm {

class HTArgs{
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

	static void putAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore);
	static void removeAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore);
	static void moveAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore);
	static void getAtomically(HyflowObject* self, void* args, HyflowContext* __context__, std::pair<int, double>* pair);

	static void putMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore);
	static void removeMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore);
	static void moveMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore);
	static void getMultiAtomically(HyflowObject* self, void* args, HyflowContext* __context__, std::pair<int, double>* ignore);

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

	static void putMulti(std::pair<int, double> entry[], int size);
	static void removeMulti(int values[], int size);
	static void moveMulti(int key1[], int key2[], int size);
	static void getMulti(int values[], int size);

	void test();
};

} /* namespace vt_dstm */

#endif /* HASHTABLE_H_ */
