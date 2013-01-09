/*
 * ListNode.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#include "ListNode.h"
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

void ListNode::addNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore) {
	int newNodeValue = *(((ListArgs*)args)->values);
	ListNode* currentNode = NULL;
	std::string head("HEAD");
	std::string prev = head, next;

	//Fetch the Head Node first, It is a sentinel Node
	HYFLOW_FETCH(head, true);
	currentNode = (ListNode*)HYFLOW_ON_READ(head);
	next = currentNode->getNextId();
	LOG_DEBUG("LIST :First Node in List is %s adding new value %d\n", next.c_str(), newNodeValue);

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<newNodeValue;
		std::string lockName = absLockStr.str();
		__context__->onLockAccess("LST0", lockName, false);
	}

	if (next.compare("NULL") == 0) {
		ListNode* newNode = new ListNode(newNodeValue, ListBenchmark::getId());
		newNode->setNextId(next);
		HYFLOW_PUBLISH_OBJECT(newNode);

		ListNode* headNodeWrite = (ListNode*)HYFLOW_ON_WRITE(head);
		headNodeWrite->setNextId(newNode->getId());
		LOG_DEBUG("LIST :In empty list set Head next Id to %s  value %d\n", newNode->getId().c_str(), newNodeValue);
	}else {
		// Find the correct place to add
		while(next.compare("NULL") != 0) {
			LOG_DEBUG("LIST :Add traverse when prev=%s and next=%s \n", prev.c_str(), next.c_str());
			HYFLOW_FETCH(next, true);
			currentNode = (ListNode*)HYFLOW_ON_READ(next);
			int nextNodeValue = currentNode->getValue();
			if (nextNodeValue > newNodeValue) {
				LOG_DEBUG("LIST :Got the required value %d to add before in node %s\n", newNodeValue, next.c_str());
				break;
			}else if (nextNodeValue == newNodeValue) {
				LOG_DEBUG("LIST :Value %d already exist in node %s\n", newNodeValue, next.c_str());
				return;
			}
			prev = next;
			next = currentNode->getNextId();
		}

		ListNode* newNode = new ListNode(newNodeValue, ListBenchmark::getId());
		newNode->setNextId(next);
		HYFLOW_PUBLISH_OBJECT(newNode);

		if (next.compare("NULL") == 0) {
			LOG_DEBUG("LIST :At end of list set %s next Id to %s value %d\n", prev.c_str(), newNode->getId().c_str(), newNodeValue);
		}
		ListNode* prevNode = (ListNode*)HYFLOW_ON_WRITE(prev);
		prevNode->setNextId(newNode->getId());
		LOG_DEBUG("LIST :Add Set %s next to %s\n", prev.c_str(), newNode->getId().c_str());
	}
}

void ListNode::addNode(int value) {
	Atomic atomicAdd;
	ListArgs largs(&value, 1);
	atomicAdd.atomically = ListNode::addNodeAtomically;
	atomicAdd.onAbort = ListNode::addAbort;
	atomicAdd.execute(NULL, &largs, NULL);
}

void ListNode::deleteNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	int givenValue = *(((ListArgs*)args)->values);
	ListReturn *lRet = (ListReturn*) rt;
	ListNode* targetNode = NULL;
	std::string head("HEAD");
	std::string prev = head, next;

	//Fetch the Head Node first, It is just a dummy Node
	HYFLOW_FETCH(head, true);
	targetNode = (ListNode*)HYFLOW_ON_READ(head);
	next = targetNode->getNextId();
	LOG_DEBUG("LIST :First Node is List %s searching for %d\n", next.c_str(), givenValue);

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<givenValue;
		std::string lockName = absLockStr.str();
		__context__->onLockAccess("LST0", lockName, false);
	}

	while(next.compare("NULL") != 0) {
		LOG_DEBUG("LIST :DEL traverse when prev=%s and next=%s \n", prev.c_str(), next.c_str());
		HYFLOW_FETCH(next, true);
		targetNode = (ListNode*)HYFLOW_ON_READ(next);
		int nodeValue = targetNode->getValue();
		if (nodeValue == givenValue) {
			ListNode* prevNode = (ListNode*)HYFLOW_ON_WRITE(prev);
			ListNode* currentNode = (ListNode*)HYFLOW_ON_WRITE(next);
			prevNode->setNextId(currentNode->getNextId());
			HYFLOW_DELETE_OBJECT(currentNode);
			lRet->success = true;
			LOG_DEBUG("LIST :Got the required value %d in node %s\n", givenValue, currentNode->getId().c_str());
			break;
		}else if (nodeValue > givenValue) {
			LOG_DEBUG("LIST :DEL object %d not found in list\n", givenValue);
			break;
		}
		prev = next;
		next = targetNode->getNextId();
	}
}

void ListNode::deleteNode(int value) {
	Atomic atomicDelete;
	ListArgs largs(&value, 1);
	ListReturn lRet;

	atomicDelete.atomically = ListNode::deleteNodeAtomically;
	atomicDelete.onAbort = ListNode::deleteAbort;
	atomicDelete.execute(NULL, &largs, &lRet);
}

void ListNode::sumNodesAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore) {
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
}

void ListNode::sumNodes() {
	Atomic atomicSum;
	atomicSum.atomically = ListNode::sumNodesAtomically;
	atomicSum.execute(NULL, NULL, NULL);
}

void ListNode::findNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore) {
	int givenValue = *(((ListArgs*)args)->values);

	ListNode* targetNode = NULL;
	std::string head("HEAD");
	std::string prev = head, next;

	//Fetch the Head Node first, It is just a dummy Node
	HYFLOW_FETCH(head, true);
	targetNode = (ListNode*)HYFLOW_ON_READ(head);
	next = targetNode->getNextId();
	LOG_DEBUG("LIST :First Node is List %s\n", next.c_str());

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<givenValue;
		std::string lockName = absLockStr.str();
		__context__->onLockAccess("LST0", lockName, true);
	}

	while(next.compare("NULL") != 0) {
		HYFLOW_FETCH(next, true);
		targetNode = (ListNode*)HYFLOW_ON_READ(next);
		int nodeValue = targetNode->getValue();
		if (nodeValue == givenValue) {
			LOG_DEBUG("LIST :Found Value %d in %s\n", nodeValue, targetNode->getId().c_str());
			break;
		}
		prev = next;
		next = targetNode->getNextId();
		//TODO: Think about removing previous->previous node from read write set
	}
}

void ListNode::findNode(int nodeValue) {
	Atomic atomicfind;
	ListArgs largs(&nodeValue, 1);

	atomicfind.atomically = ListNode::findNodeAtomically;
	atomicfind.execute(NULL, &largs, NULL);
}

void ListNode::addNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
	ListArgs* largs = (ListArgs*)args;
	for (int txns = 0; txns < largs->size ; txns+=1) {
		addNode(largs->values[txns]);
	}
}

void ListNode::deleteNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
	ListArgs* largs = (ListArgs*)args;
	for (int txns = 0; txns < largs->size ; txns+=1) {
		deleteNode(largs->values[txns]);
	}
}

void ListNode::sumNodesMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
	ListArgs* largs = (ListArgs*)args;
	for (int txns = 0; txns < largs->size ; txns+=1) {
		sumNodes();
	}
}

void ListNode::findNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
	ListArgs* largs = (ListArgs*)args;
	for (int txns = 0; txns < largs->size ; txns+=1) {
		findNode(largs->values[txns]);
	}
}

void ListNode::addNodeMulti(int values[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("List :Call Add Node with %d in txns %d\n", values[txns], txns);
				addNode(values[txns]);

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		ListArgs largs(values, size);
		Atomic atomicAddMulti;
		atomicAddMulti.atomically = ListNode::addNodeMultiAtomically;
		atomicAddMulti.execute(NULL, &largs, NULL);
	}
}

void ListNode::deleteNodeMulti(int values[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("List :Call Delete Node with %d in txns %d\n", values[txns], txns);
				deleteNode(values[txns]);

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		ListArgs largs(values, size);
		Atomic atomicDeleteMulti;
		atomicDeleteMulti.atomically = ListNode::deleteNodeMultiAtomically;
		atomicDeleteMulti.execute(NULL, &largs, NULL);
	}
}

void ListNode::sumNodesMulti(int count) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<count ; txns+=1 ) {
				LOG_DEBUG("List :Call sumNodes %d  time\n", txns);
				sumNodes();

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		ListArgs largs(NULL, count);
		Atomic atomicSumMulti;
		atomicSumMulti.atomically = ListNode::sumNodesMultiAtomically;
		atomicSumMulti.execute(NULL, &largs, NULL);
	}
}

void ListNode::findNodeMulti(int values[], int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("List :Call find Node with %d in txns %d\n", values[txns], txns);
				findNode(values[txns]);

				if (txns%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		ListArgs largs(values, size);
		Atomic atomicfindMulti;
		atomicfindMulti.atomically = ListNode::findNodeMultiAtomically;
		atomicfindMulti.execute(NULL, &largs, NULL);
	}
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

void ListNode::addAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	int givenValue = *(((ListArgs*)args)->values);
	bool found = false;

	ListNode* targetNode = NULL;
	std::string head("HEAD");
	std::string prev = head, next;

	//Fetch the Head Node first, It is just a dummy Node
	HYFLOW_FETCH(head, true);
	targetNode = (ListNode*)HYFLOW_ON_READ(head);
	next = targetNode->getNextId();
	LOG_DEBUG("LIST :First Node is List %s searching for %d\n", next.c_str(), givenValue);

	while(next.compare("NULL") != 0) {
		LOG_DEBUG("LIST :DEL traverse when prev=%s and next=%s \n", prev.c_str(), next.c_str());
		HYFLOW_FETCH(next, true);
		targetNode = (ListNode*)HYFLOW_ON_READ(next);
		int nodeValue = targetNode->getValue();
		if (nodeValue == givenValue) {
			ListNode* prevNode = (ListNode*)HYFLOW_ON_WRITE(prev);
			ListNode* currentNode = (ListNode*)HYFLOW_ON_WRITE(next);
			prevNode->setNextId(currentNode->getNextId());
			HYFLOW_DELETE_OBJECT(currentNode);
			found = true;
			LOG_DEBUG("LIST :Got the required value %d in node %s\n", givenValue, currentNode->getId().c_str());
			break;
		}else if (nodeValue > givenValue) {
			LOG_DEBUG("LIST :DEL object %d not found in list\n", givenValue);
			break;
		}
		prev = next;
		next = targetNode->getNextId();
	}

	if (!found) {
		Logger::fatal("List :AddAbort unable to delete the %d in open Nesting\n", givenValue);
	}
}

void ListNode::deleteAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	int newNodeValue = *(((ListArgs*)args)->values);
	ListReturn *lRet = (ListReturn*) rt;

	if (!lRet->success) {
		return;
	}

	ListNode* currentNode = NULL;
	std::string head("HEAD");
	std::string prev = head, next;

	//Fetch the Head Node first, It is a sentinel Node
	HYFLOW_FETCH(head, true);
	currentNode = (ListNode*)HYFLOW_ON_READ(head);
	next = currentNode->getNextId();
	LOG_DEBUG("LIST :First Node in List is %s adding new value %d\n", next.c_str(), newNodeValue);

	if (next.compare("NULL") == 0) {
		ListNode* newNode = new ListNode(newNodeValue, ListBenchmark::getId());
		newNode->setNextId(next);
		HYFLOW_PUBLISH_OBJECT(newNode);

		ListNode* headNodeWrite = (ListNode*)HYFLOW_ON_WRITE(head);
		headNodeWrite->setNextId(newNode->getId());
		LOG_DEBUG("LIST :In empty list set Head next Id to %s  value %d\n", newNode->getId().c_str(), newNodeValue);
	}else {
		// Find the correct place to add
		while(next.compare("NULL") != 0) {
			LOG_DEBUG("LIST :Add traverse when prev=%s and next=%s \n", prev.c_str(), next.c_str());
			HYFLOW_FETCH(next, true);
			currentNode = (ListNode*)HYFLOW_ON_READ(next);
			int nextNodeValue = currentNode->getValue();
			if (nextNodeValue > newNodeValue) {
				LOG_DEBUG("LIST :Got the required value %d to add before in node %s\n", newNodeValue, next.c_str());
				break;
			}else if (nextNodeValue == newNodeValue) {
				Logger::fatal("LIST :Error value %d already exist in node %s\n", newNodeValue, next.c_str());
				return;
			}
			prev = next;
			next = currentNode->getNextId();
		}

		ListNode* newNode = new ListNode(newNodeValue, ListBenchmark::getId());
		newNode->setNextId(next);
		HYFLOW_PUBLISH_OBJECT(newNode);

		if (next.compare("NULL") == 0) {
			LOG_DEBUG("LIST :At end of list set %s next Id to %s value %d\n", prev.c_str(), newNode->getId().c_str(), newNodeValue);
		}
		ListNode* prevNode = (ListNode*)HYFLOW_ON_WRITE(prev);
		prevNode->setNextId(newNode->getId());
		LOG_DEBUG("LIST :Add Set %s next to %s\n", prev.c_str(), newNode->getId().c_str());
	}
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

