/*
 * ListNode.h
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LISTNODE_H_
#define LISTNODE_H_

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/helper/BenchMarkArgs.h"
#include "../../../core/helper/BenchMarkReturn.h"

namespace vt_dstm {

class ListArgs: public BenchMarkArgs {
public:
	int *values;
	int size;

	ListArgs(int *v, int s) {
		values = v ;
		size = s ;
		onHeap = false;
	}

	~ListArgs() {
		if (onHeap) {
			delete[] values;
		}
	}

	void getClone(BenchMarkArgs** arg) {
		int* val = new int[size];
		for (int i = 0 ; i<size ; i++) {
			val[i] = values[i];
		}
		ListArgs* clone = new ListArgs(val, size);
		clone->onHeap = true;
		*arg = clone;
	}
};

class ListReturn: public BenchMarkReturn {
public:
	bool success;

	ListReturn() {
		success = false;
		onHeap = false;
	}

	void getClone(BenchMarkReturn **benchArgs) {
		ListReturn* lrt = new ListReturn();
		lrt->success = success;
		lrt->onHeap = true;
		*benchArgs = lrt;
	}
};

class ListNode: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & nextId;
    	ar & value;
    }

	std::string nextId;
	int value;
	static void addNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* success);
	static void sumNodesAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	static void addNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void sumNodesMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	/*
	 * Deletes the first occurrence of value in list
	 */
	static void deleteNode(int value);
	/*
	 * Sums all the values in the list
	 */
	static void sumNodes();
	/*
	 * Finds the first occurrence of value in list
	 */
	static void findNode(int value);
public:
	ListNode();
	ListNode(int value, int counter);
	ListNode(int value, std::string id);
	virtual ~ListNode();

	std::string getNextId() const;
	void setNextId(std::string nextId);
	int getValue() const;
	void setValue(int value);
	void print();
	void getClone(HyflowObject **obj);

	/*
	 * Adds the given value in the list
	 */
	static void addNode(int value);
	static void addNodeMulti(int values[], int size);
	static void deleteNodeMulti(int values[], int size);
	static void sumNodesMulti(int count);
	static void findNodeMulti(int values[], int size);

	static void addAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);
	static void deleteAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);


	void test();
};

} /* namespace vt_dstm */

#endif /* LISTNODE_H_ */
