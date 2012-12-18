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
	}

	void getClone(BenchMarkArgs** arg) {

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
	static void deleteNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void sumNodesAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	static void addNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void deleteNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void sumNodesMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void findNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	/*
	 * Adds the given value in the list
	 */
	static void addNode(int value);
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

	static void addNodeMulti(int values[], int size);
	static void deleteNodeMulti(int values[], int size);
	static void sumNodesMulti(int count);
	static void findNodeMulti(int values[], int size);

	void test();
};

} /* namespace vt_dstm */

#endif /* LISTNODE_H_ */
