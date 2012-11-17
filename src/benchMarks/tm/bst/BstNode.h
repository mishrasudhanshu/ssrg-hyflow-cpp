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

namespace vt_dstm {

class BstNode: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & leftChild;
    	ar & rightChild;
    	ar & value;
    }

	std::string leftChild;
	std::string rightChild;
	int value;
	static void addNode(int value, HyflowContext *c, HyflowObjectFuture & fu);
	static void deleteNode(int value, HyflowContext *c);
	static void sumNodes(HyflowContext *c);
	static void findNode(int value, HyflowContext *c);
public:
	BstNode();
	BstNode(int value, int counter);
	BstNode(int value, std::string id);
	virtual ~BstNode();

	int getValue() const;
	void setValue(int value);
	std::string getLeftChild() const;
	void setLeftChild(std::string leftChild);
	std::string getRightChild() const;
	void setRightChild(std::string rightChild);
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
	 * Sums all the values in the list
	 */
	static void sumNodes();
	/*
	 * Finds the first occurrence of value in list
	 */
	static void findNode(int value);

	void test();
};

} /* namespace vt_dstm */

#endif /* BSTNODE_H_ */
