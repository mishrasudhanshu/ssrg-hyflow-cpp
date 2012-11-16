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
	hyVersion = 0;
}

ListNode::ListNode(int val, std::string id) {
	value = val;
	hyId = id;
	hyVersion = 0;
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
		HYFLOW_FETCH(head, false);

		ListNode* headNodeRead =  (ListNode*)HYFLOW_ON_READ(head);
		std::string oldNext = headNodeRead->getNextId();
		ListNode* newNode = new ListNode(value, ListBenchmark::getId());
		newNode->setNextId(oldNext);
		HYFLOW_PUBLISH_OBJECT(newNode);

		ListNode* headNodeWrite = (ListNode*)HYFLOW_ON_WRITE(head);
		headNodeWrite->setNextId(newNode->getId());
		LOG_DEBUG("LIST :Set Head next Id to %s value %d\n", newNode->getId().c_str(), value);
	} HYFLOW_ATOMIC_END;
}

void ListNode::deleteNode(int value, HyflowContext *c) {}

void ListNode::deleteNode(int value) {
	HYFLOW_ATOMIC_START{
		ListNode* targetNode = NULL;
		std::string head("HEAD");
		std::string prev = head, next;
		//Fetch the Head Node first, It is just a dummy Node
		HYFLOW_FETCH(head, true);
		targetNode = (ListNode*)HYFLOW_ON_READ(head);
		next = targetNode->getNextId();
		LOG_DEBUG("LIST :First Node is List %s searching for %d\n", next.c_str(), value);

		while(next.compare("NULL") != 0) {
			HYFLOW_FETCH(next, true);
			targetNode = (ListNode*)HYFLOW_ON_READ(next);
			int nodeValue = targetNode->getValue();
			if (nodeValue == value) {
				ListNode* prevNode = (ListNode*)HYFLOW_ON_WRITE(prev);
				ListNode* currentNode = (ListNode*)HYFLOW_ON_WRITE(next);
				prevNode->setNextId(currentNode->getNextId());
				HYFLOW_DELETE_OBJECT(currentNode);
				LOG_DEBUG("LIST :Got the required value %d in node %s\n", value, currentNode->getId().c_str());
				break;
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
		HYFLOW_FETCH(head, true);
		targetNode = (ListNode*)HYFLOW_ON_READ(head);
		next = targetNode->getNextId();
		LOG_DEBUG("LIST :First Node is List %s\n", next.c_str());

		while(next.compare("NULL") != 0) {
			HYFLOW_FETCH(next, true);
			targetNode = (ListNode*)HYFLOW_ON_READ(next);
			nodeSum += targetNode->getValue();
			prev = next;
			next = targetNode->getNextId();
			//TODO: Think about removing previous->previous node from read write set
			LOG_DEBUG("LIST :Got value %d in %s with next %s\n", targetNode->getValue(), targetNode->getId().c_str(), next.c_str());
		}
		LOG_DEBUG("LIST :Sum Value=%d\n", nodeSum);
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
		HYFLOW_FETCH(head, true);
		targetNode = (ListNode*)HYFLOW_ON_READ(head);
		next = targetNode->getNextId();
		LOG_DEBUG("LIST :First Node is List %s\n", next.c_str());

		while(next.compare("NULL") != 0) {
			HYFLOW_FETCH(next, true);
			targetNode = (ListNode*)HYFLOW_ON_READ(next);
			int nodeValue = targetNode->getValue();
			if (nodeValue == value) {
				isPresent = true;
				LOG_DEBUG("LIST :Found Value %d in %s\n", nodeValue, targetNode->getId().c_str());
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
	ListNode *ln = new ListNode();
	ln->nextId = nextId;
	ln->value = value;
	this->baseClone(ln);
	*obj = ln;
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

