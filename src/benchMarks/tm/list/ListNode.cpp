/*
 * ListNode.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#include <string>
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "ListNode.h"
#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "ListBenchmark.h"


namespace vt_dstm {

ListNode::ListNode() {}

ListNode::ListNode(int val, int counter) {
	value = val;
	std::stringstream idStr;
	int ownerNode = NetworkManager::getNodeId();
	idStr<<ownerNode<<"-"<<counter;
	hyId = idStr.str();
}

ListNode::ListNode(int val, std::string id) {
	value = val;
	hyId = id;
}

std::string ListNode::getNextId() const {
	return nextId;
}

void ListNode::setNextId(std::string nextId) {
	this->nextId = nextId;
}

int ListNode::getValue() const {
	return value;
}

void ListNode::setValue(int value) {
	this->value = value;
}

ListNode::~ListNode() {
}

void ListNode::addNode(int value, HyflowContext *c, HyflowObjectFuture & fu) {}

void ListNode::addNode(int value) {
	HYFLOW_ATOMIC_START{
		std::string head="HEAD";
		__context__->fetchObject(head, false);

		ListNode* headNodeRead =  (ListNode*)__context__->onReadAccess(head);
		std::string oldNext = headNodeRead->getNextId();
		ListNode* newNode = new ListNode(value, ListBenchmark::getId());
		newNode->setNextId(oldNext);
		__context__->addToPublish(newNode);

		ListNode* headNodeWrite = (ListNode*)__context__->onWriteAccess(head);
		headNodeWrite->setId(newNode->getId());
	} HYFLOW_ATOMIC_END;
}

void ListNode::deleteNode(int value, HyflowContext *c) {}

void ListNode::deleteNode(int value) {
	HYFLOW_ATOMIC_START{
		ListNode* targetNode = NULL;
		std::string head("HEAD");
		std::string prev = head, next;
		//Fetch the Head Node first, It is just a dummy Node
		__context__->fetchObject(head, true);
		targetNode = (ListNode*)__context__->onReadAccess(head);
		next = targetNode->getNextId();

		while(next.compare("NULL") != 0) {
			__context__->fetchObject(next, true);
			targetNode = (ListNode*)__context__->onReadAccess(next);
			int nodeValue = targetNode->getValue();
			if (nodeValue == value) {
				LOG_DEBUG("LIST :Got the required value node\n");
				ListNode* prevNode = (ListNode*)__context__->onWriteAccess(prev);
				ListNode* currentNode = (ListNode*)__context__->onWriteAccess(next);
				prevNode->setNextId(currentNode->getNextId());
				__context__->addToDelete(currentNode);
			}
			prev = next;
			next = targetNode->getNextId();
			//TODO: Think about removing previous->previous node from read write set
		}
	} HYFLOW_ATOMIC_END;
}

void ListNode::sumNodes(HyflowContext *c) {}

void ListNode::sumNodes() {
	HYFLOW_ATOMIC_START{
		ListNode* targetNode = NULL;
		std::string head("HEAD");
		std::string prev = head, next;
		int nodeSum =0 ;
		//Fetch the Head Node first, It is just a dummy Node
		__context__->fetchObject(head, true);
		targetNode = (ListNode*)__context__->onReadAccess(head);
		next = targetNode->getNextId();

		while(next.compare("NULL") != 0) {
			__context__->fetchObject(next, true);
			targetNode = (ListNode*)__context__->onReadAccess(next);
			nodeSum += targetNode->getValue();
			prev = next;
			next = targetNode->getNextId();
			//TODO: Think about removing previous->previous node from read write set
		}
	} HYFLOW_ATOMIC_END;
}

void ListNode::findNode(int value, HyflowContext *c) {

}

void ListNode::findNode(int value) {
	HYFLOW_ATOMIC_START{
		bool isPresent = false;
		ListNode* targetNode = NULL;
		std::string head("HEAD");
		std::string prev = head, next;
		//Fetch the Head Node first, It is just a dummy Node
		__context__->fetchObject(head, true);
		targetNode = (ListNode*)__context__->onReadAccess(head);
		next = targetNode->getNextId();

		while(next.compare("NULL") != 0) {
			__context__->fetchObject(next, true);
			targetNode = (ListNode*)__context__->onReadAccess(next);
			int nodeValue = targetNode->getValue();
			if (nodeValue == value) {
				isPresent = true;
				break;
			}
			prev = next;
			next = targetNode->getNextId();
			//TODO: Think about removing previous->previous node from read write set
		}
	} HYFLOW_ATOMIC_END;
}

void ListNode::print() {

}

void ListNode::getClone(HyflowObject **obj) {

}

void ListNode::test() {
	// create and open a character archive for output
	std::ofstream ofs("List", std::ios::out);

	// create class instance
	std::string id="0-0";
	vt_dstm::ListNode  l;
	l.setId(id);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << l;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::ListNode l1;
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

