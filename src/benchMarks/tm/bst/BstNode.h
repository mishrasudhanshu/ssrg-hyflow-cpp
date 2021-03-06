/*
 * BstNode.h
 *
 *  Created on: Nov 16, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BSTNODE_H_
#define BSTNODE_H_

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/helper/BenchMarkArgs.h"
#include "../../../core/helper/BenchMarkReturn.h"

#define HYFLOW_BST_COUNT 3

namespace vt_dstm {

class BstArgs: public BenchMarkArgs {
public:
	int *values;
	int size;

	BstArgs(int *v, int s) {
		values = v ;
		size = s ;
		onHeap = false;
	}

	~BstArgs() {
		if (onHeap) {
			delete[] values;
		}
	}

	void getClone(BenchMarkArgs** arg) {
		int* val = new int[size];
		for (int i = 0 ; i<size ; i++) {
			val[i] = values[i];
		}
		BstArgs* clone = new BstArgs(val, size);
		clone->onHeap = true;
		*arg = clone;
	}
};

class BstReturn: public BenchMarkReturn {
public:
	int success;

	BstReturn() {
		success = 0;
		onHeap = false;
	}

	void getClone(BenchMarkReturn **benchArgs) {
		BstReturn* lrt = new BstReturn();
		lrt->success = success;
		lrt->onHeap = true;
		*benchArgs = lrt;
	}
};

class BstNode: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & leftChild;
    	ar & rightChild;
    	ar & value;
    	ar & bstNumber;
    }

    static void addNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	/*
	 * Deletes the first occurrence of value in bst
	 */
	static void deleteNode(int val);
	/*
	 * Finds the first occurrence of value in bst
	 */
	static void findNode(int val);

	static void addNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

public:
	std::string leftChild;
	std::string rightChild;
	int value;
	int bstNumber;

	BstNode();
	BstNode(int value, int counter, int bstN);
	BstNode(int value, std::string id, int bstN);

	virtual ~BstNode();

	/*
	 * Adds the given value in the bst
	 */
	static void addNode(int val);
	int getValue() const;
	void setValue(int value);
	std::string getLeftChild() const;
	void setLeftChild(std::string leftChild);
	std::string getRightChild() const;
	void setRightChild(std::string rightChild);
	void print();
	void getClone(HyflowObject **obj);

	static void addNodeMulti(int values[], int size);
	static void deleteNodeMulti(int values[], int size);
	static void findNodeMulti(int values[], int size);

	static void addAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);
	static void deleteAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt);

	void test();
};

} /* namespace vt_dstm */

#endif /* BSTNODE_H_ */
