/*
 * BstNode.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: mishras[at]vt.edu
 */

#include "BstNode.h"

#include <string>
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../BenchmarkExecutor.h"
#include "BstBenchmark.h"

#define ROOT "HEAD"

namespace vt_dstm {

BstNode::BstNode() {}

BstNode::BstNode(int val, int counter) {
	value = val;
	std::stringstream idStr;
	int ownerNode = NetworkManager::getNodeId();
	idStr<<ownerNode<<"-"<<counter;
	hyId = idStr.str();
	hyVersion = 0;
	rightChild = "NULL";
	leftChild = "NULL";
}

BstNode::BstNode(int val, std::string id) {
	value = val;
	hyId = id;
	hyVersion = 0;
	rightChild = "NULL";
	leftChild = "NULL";
}

int BstNode::getValue() const {
	return value;
}

void BstNode::setValue(int value) {
	this->value = value;
}

BstNode::~BstNode() {
}

void BstNode::addNode(int value, HyflowContext *c, HyflowObjectFuture & fu) {}

void BstNode::addNode(int value) {
	HYFLOW_ATOMIC_START {
		HYFLOW_CHECKPOINT_INIT;
		std::string head="HEAD";
		HYFLOW_FETCH(head, true);

		// Dummy Node whose right Child points to Root Node
		BstNode* headNode = (BstNode*) HYFLOW_ON_READ(head);
		std::string root = headNode->rightChild;
		LOG_DEBUG("BST :Root node is %s \n", root.c_str());

		BstNode* newNode = new BstNode(value, BstBenchmark::getId());
		// Check if Root Node exists or not
		if (root.compare("NULL")== 0) {
			// Create given value as root Node
			headNode = (BstNode*) HYFLOW_ON_WRITE(head);
			headNode->rightChild = newNode->getId();
			LOG_DEBUG("BST :Added %s with value %d at root\n", newNode->getId().c_str(), newNode->value);
		}else {
			BstNode* currentNode = NULL;
			std::string next = root;
			bool leftChild = false;
			do {
				HYFLOW_FETCH(next, true);
				currentNode = (BstNode*) HYFLOW_ON_READ(next);
				if(currentNode->value > value) {
					next = currentNode->leftChild;
					leftChild = true;
					LOG_DEBUG("BST :Node %s with value %d move child %d to left, towards %s\n", currentNode->getId().c_str(), currentNode->value, value, next.c_str());
				}else {
					next = currentNode->rightChild;
					leftChild = false;
					LOG_DEBUG("BST :Node %s with value %d move child %d to right, towards %s\n", currentNode->getId().c_str(), currentNode->value, value, next.c_str());
				}
			}while (next.compare("NULL") != 0);

			// Now Current Node contains the parent Node for new child
			currentNode = (BstNode*) HYFLOW_ON_WRITE(currentNode->getId());
			if(leftChild) {
				currentNode->leftChild = newNode->getId();
			}else {
				currentNode->rightChild = newNode->getId();
			}
		}
		HYFLOW_PUBLISH_OBJECT(newNode);
	}HYFLOW_ATOMIC_END;
}

void BstNode::deleteNode(int value, HyflowContext* c) {
}

void BstNode::deleteNode(int value) {
	HYFLOW_ATOMIC_START {
		std::string head="HEAD";
		HYFLOW_FETCH(head, true);

		// Sentinel Node whose right Child points to Root Node
		BstNode* headNode = (BstNode*) HYFLOW_ON_READ(head);
		std::string root = headNode->rightChild;
		LOG_DEBUG("BST :Root node is %s\n", root.c_str());

		// Check if Root Node exists or not
		if (root.compare("NULL")!= 0) {	//otherwise nothing to delete
			std::string currentNodeId = "NULL", prevNodeId = "NULL";
			std::string next = root;

			bool leftChild = false;
			do {
				prevNodeId = currentNodeId;
				currentNodeId = next;

				HYFLOW_FETCH(currentNodeId, true);
				BstNode	*currentNode= (BstNode*)HYFLOW_ON_READ(currentNodeId);

				if(currentNode->value > value) {
					next = currentNode->getLeftChild();
					leftChild = true;
					LOG_DEBUG("BST :Node %s with value %d move child %d to left towards %s\n", currentNode->getId().c_str(), currentNode->value, value, next.c_str());
				}else if(currentNode->value < value) {
					next = currentNode->getRightChild();
					leftChild = false;
					LOG_DEBUG("BST :Node %s with value %d move child %d to right towards %s\n", currentNode->getId().c_str(), currentNode->value, value, next.c_str());
				}else {
					std::string replacementId;
					if(currentNode->leftChild.compare("NULL")==0){
						replacementId  = currentNode->rightChild;
						LOG_DEBUG("BST :Got replacement %s in rightChild\n", replacementId.c_str());
					}else if(currentNode->rightChild.compare("NULL")==0){
						replacementId = currentNode->leftChild;
						LOG_DEBUG("BST :Got replacement %s in leftChild\n", replacementId.c_str());
					}else{
						// Replace with left most child in right tree
						std::string cNodeId = "NULL", pNodeId = "NULL";
						std::string leftChildId = currentNode->rightChild;
						do {
							pNodeId = cNodeId;
							cNodeId = leftChildId;

							HYFLOW_FETCH(cNodeId, true);
							BstNode* cNode = (BstNode*) HYFLOW_ON_READ(cNodeId);
							leftChildId = cNode->leftChild;
						}while (leftChildId.compare("NULL")!=0);

						// We have reached to left most node in right tree
						replacementId = cNodeId;
						LOG_DEBUG("BST :Got replacement %s in leftChild of right tree of\n", replacementId.c_str());
						// Disconnect the cNode
						if (pNodeId.compare("NULL") != 0) {
							BstNode *pNode = (BstNode*) HYFLOW_ON_WRITE(pNodeId);
							BstNode *toBeDeletedNode = (BstNode*) HYFLOW_ON_WRITE(cNodeId);
							pNode->leftChild = toBeDeletedNode->rightChild;
							LOG_DEBUG("BST :Moved Node %s left child from %s to %s\n", pNode->getId().c_str(), cNodeId.c_str(), toBeDeletedNode->rightChild.c_str());
						}
					}

					// LESSON: Don't use the Node object error prone, access object using IDs
					if ((currentNode->leftChild.compare("NULL")!=0)&&(currentNode->rightChild.compare("NULL")!=0)) {
						HYFLOW_FETCH(replacementId, false);
						BstNode* replacementNode = (BstNode*)HYFLOW_ON_WRITE(replacementId);
						BstNode* toBeDeletedNode = (BstNode*)HYFLOW_ON_WRITE(currentNodeId);
						// Cases in which replaced nodes are child of deleted node
						if (toBeDeletedNode->leftChild.compare(replacementId) != 0) { // For left child condition not required
							replacementNode->leftChild = toBeDeletedNode->leftChild;
						}
						if (toBeDeletedNode->rightChild.compare(replacementId) != 0) {
							replacementNode->rightChild = toBeDeletedNode->rightChild;
						}
						LOG_DEBUG("BST :Set %s left child %s to %s and right child %s to %s if condition met\n", replacementId.c_str(), replacementNode->leftChild.c_str(), toBeDeletedNode->leftChild.c_str(), replacementNode->rightChild.c_str(), toBeDeletedNode->rightChild.c_str());
					}

					if(prevNodeId.compare("NULL") != 0 ) {
						BstNode *prevNode = (BstNode*) HYFLOW_ON_WRITE(prevNodeId);
						if(leftChild) {
							prevNode->leftChild = replacementId;
							LOG_DEBUG("BST :DEL update prevNode %s leftChild %s\n",prevNode->getId().c_str(), replacementId.c_str());
						}else {
							prevNode->rightChild = replacementId;
							LOG_DEBUG("BST :DEL update prevNode %s rightChild %s\n",prevNode->getId().c_str(), replacementId.c_str());
						}
					}else {
						// We delete root node itself, set our sentinel head to NULL
						headNode = (BstNode*) HYFLOW_ON_WRITE(head);
						headNode->rightChild = "NULL";
					}
					HYFLOW_DELETE_OBJECT(currentNode);
					break;
				}
			}while(next.compare("NULL")!=0);
		}else {
			LOG_DEBUG("BST :Nothing to Delete\n");
		}
	}HYFLOW_ATOMIC_END;
}

void BstNode::findNode(int value, HyflowContext* c) {
}

void BstNode::findNode(int value) {
	HYFLOW_ATOMIC_START {
		std::string head="HEAD";
		HYFLOW_FETCH(head, true);

		// Dummy Node whose right Child points to Root Node
		BstNode* headNode = (BstNode*) HYFLOW_ON_READ(head);
		std::string root = headNode->rightChild;
		LOG_DEBUG("BST :Root node is %s\n", root.c_str());

		// Check if Root Node exists or not
		if (root.compare("NULL") != 0) {
//			HYFLOW_FETCH(root, true);
			BstNode* currentNode = NULL;
			std::string next = root;
			do {
				HYFLOW_FETCH(next, true);
				currentNode = (BstNode*) HYFLOW_ON_READ(next);
				if(currentNode->value > value) {
					next = currentNode->leftChild;
					LOG_DEBUG("BST :Node %s with value %d move child %d to left towards %s\n", currentNode->getId().c_str(), currentNode->value, value, next.c_str());
				} else if(currentNode->value < value) {
					next = currentNode->rightChild;
					LOG_DEBUG("BST :Node %s with value %d move child %d to right towards %s\n", currentNode->getId().c_str(), currentNode->value, value, next.c_str());
				} else {
					LOG_DEBUG("BST :Found Object %s with value %d\n", currentNode->getId().c_str(), value);
					break;
				}
			}while (next.compare("NULL") != 0);
		}else {
			LOG_DEBUG("BST :Nothing to find\n");
		}
	}HYFLOW_ATOMIC_END;
}

void BstNode::print() {
}

std::string BstNode::getLeftChild() const {
	return leftChild;
}

void BstNode::setLeftChild(std::string leftChild) {
	this->leftChild = leftChild;
}

std::string BstNode::getRightChild() const {
	return rightChild;
}

void BstNode::setRightChild(std::string rightChild) {
	this->rightChild = rightChild;

}

void BstNode::getClone(HyflowObject **obj) {
	BstNode *ln = new BstNode();
	ln->leftChild = leftChild;
	ln->rightChild = rightChild;
	ln->value = value;
	this->baseClone(ln);
	*obj = ln;
}

void BstNode::test() {
	// create and open a character archive for output
	std::ofstream ofs("List", std::ios::out);

	// create class instance
	std::string id="0-0";
	vt_dstm::BstNode  l;
	l.setId(id);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << l;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::BstNode l1;
	{
		// create and open an archive for input
		std::ifstream ifs("List", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> l1;
		// archive and stream closed when destructors are called
		l1.print();
	}
}

} /* namespace vt_dstm */
