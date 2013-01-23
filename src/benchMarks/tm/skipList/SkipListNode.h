/*
 * SkipListNode.h
 *
 *  Created on: Nov 15, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef SKIPLISTNODE_H_
#define SKIPLISTNODE_H_

#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/helper/BenchMarkArgs.h"
#include "../../../core/helper/BenchMarkReturn.h"

#define HYFLOW_SKIPLIST_COUNT 3

namespace vt_dstm {

class SkipListArgs: public BenchMarkArgs {
public:
	int *values;
	int size;

	SkipListArgs(int *v, int s) {
		values = v ;
		size = s ;
	}

	~SkipListArgs() {
		if (onHeap) {
			delete[] values;
		}
	}

	void getClone(BenchMarkArgs ** args) {
		int* val = new int[size];
		for (int i = 0 ; i<size ; i++) {
			val[i] = values[i];
		}
		SkipListArgs* clone = new SkipListArgs(val, size);
		clone->onHeap = true;
		*args = clone;
	}
};

class SkipListReturn: public BenchMarkReturn {
public:
	int success;

	SkipListReturn() {
		success = 0;
		onHeap = false;
	}

	void getClone(BenchMarkReturn **benchArgs) {
		SkipListReturn* lrt = new SkipListReturn();
		lrt->success = success;
		lrt->onHeap = true;
		*benchArgs = lrt;
	}
};

class SkipListNode: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & nextIds;
    	ar & value;
    	ar & highestLevel;
    	ar & listNumber;
    }

	std::vector<std::string> nextIds;
	int listNumber;
	int value;
	int highestLevel;

	static void addNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	static void addNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	static void deleteNode(int value);
	static void findNode(int value);
	static int getRandomLevel();
public:
	SkipListNode();
	SkipListNode(int value, int counter, int listN);
	SkipListNode(int value, std::string id, int levels, int listN);
	virtual ~SkipListNode();

	std::string getNextId(int index) const;
	void setNextId(std::string nextId, int index);
	int getValue() const;
	void setValue(int value);
	void print();
	void getClone(HyflowObject **obj);
	static void addNode(int value);
	/*
	 * Adds the given value in the list
	 */
	static void addNodeMulti(int* values, int size);
	/*
	 * Deletes the first occurrence of value in list
	 */
	static void deleteNodeMulti(int* values, int size);
	/*
	 * Finds the first occurrence of value in list
	 */
	static void findNodeMulti(int* values, int size);

	static void addAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);
	static void deleteAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);

	void test();
};

} /* namespace vt_dstm */

#endif /* SKIPLISTNODE_H_ */
