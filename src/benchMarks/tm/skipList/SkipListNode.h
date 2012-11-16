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
	static void addNode(int value, HyflowContext *c, HyflowObjectFuture & fu);
	static void deleteNode(int value, HyflowContext *c);
	static void sumNodes(HyflowContext *c);
	static void findNode(int value, HyflowContext *c);
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
	static void addNode(int value);
	/*
	 * Deletes the first occurrence of value in list
	 */
	static void deleteNode(int value);
	/*
	 * Finds the first occurrence of value in list
	 */
	static void findNode(int value);

	void test();
};

} /* namespace vt_dstm */

#endif /* SKIPLISTNODE_H_ */
