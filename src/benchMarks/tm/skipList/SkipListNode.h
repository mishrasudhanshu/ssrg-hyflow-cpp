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

namespace vt_dstm {

class SkipListArgs {
public:
	int *values;
	int size;

	SkipListArgs(int *v, int s) {
		values = v ;
		size = s ;
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
    }

	std::vector<std::string> nextIds;
	int value;
	int highestLevel;

	static void addNodeAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);
	static void deleteNodeAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);
	static void findNodeAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);

	static void addNodeMultiAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);
	static void deleteNodeMultiAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);
	static void findNodeMultiAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);

	static void addNode(int value);
	static void deleteNode(int value);
	static void findNode(int value);
public:
	SkipListNode();
	SkipListNode(int value, int counter);
	SkipListNode(int value, std::string id, int levels);
	SkipListNode(int value, std::string id);
	virtual ~SkipListNode();

	std::string getNextId(int index) const;
	void setNextId(std::string nextId, int index);
	int getValue() const;
	void setValue(int value);
	void print();
	void getClone(HyflowObject **obj);
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

	void test();
};

} /* namespace vt_dstm */

#endif /* SKIPLISTNODE_H_ */
