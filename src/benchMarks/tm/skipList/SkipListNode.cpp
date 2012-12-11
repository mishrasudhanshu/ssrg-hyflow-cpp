/*
 * SkipListNode.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: mishras[at]vt.edu
 */

#include <string>
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "SkipListNode.h"
#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "SkipListBenchmark.h"


namespace vt_dstm {

SkipListNode::SkipListNode() {}

SkipListNode::SkipListNode(int val, int counter) {
	value = val;
	std::stringstream idStr;
	int ownerNode = NetworkManager::getNodeId();
	idStr<<ownerNode<<"-"<<counter;
	hyId = idStr.str();
	hyVersion = 0;
	highestLevel = (Logger::getCurrentMicroSec()%SkipListBenchmark::getSkipListLevels())+1;
	// Level 0 will contain the nodeId itself
	nextIds.push_back(hyId);
	for(int level=1; level<=highestLevel; level++) {
		nextIds.push_back("NULL");
	}
}

SkipListNode::SkipListNode(int val, std::string id, int levels) {
	value = val;
	hyId = id;
	hyVersion = 0;
	highestLevel = SkipListBenchmark::getSkipListLevels();
	nextIds.push_back(hyId);
	for(int level=1; level<=highestLevel; level++) {
		nextIds.push_back("NULL");
	}
}

SkipListNode::SkipListNode(int val, std::string id) {
	value = val;
	hyId = id;
	hyVersion = 0;
	highestLevel = (Logger::getCurrentMicroSec()%SkipListBenchmark::getSkipListLevels())+1;
	nextIds.push_back(hyId);
	for(int level=1; level<=highestLevel; level++) {
		nextIds.push_back("NULL");
	}
}

std::string SkipListNode::getNextId(int index) const {
	return nextIds[index];
}

void SkipListNode::setNextId(std::string nextId, int index) {
	this->nextIds[index] = nextId;
}

int SkipListNode::getValue() const {
	return value;
}

void SkipListNode::setValue(int value) {
	this->value = value;
}

SkipListNode::~SkipListNode() {}

void SkipListNode::addNodeAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	int givenValue = *((int*)args);

	std::string head="HEAD";
	HYFLOW_FETCH(head, false);

	SkipListNode* prevNode =  (SkipListNode*)HYFLOW_ON_READ(head);
	SkipListNode* currentNode = NULL;
	SkipListNode* newNode = new SkipListNode(givenValue, SkipListBenchmark::getId());

	//find correct place of object to insert
	for(int level=SkipListBenchmark::getSkipListLevels() ; level>0 ; level--) {
		//For given level move until you find the node with just small value
		std::string nextNodeId = prevNode->getNextId(level);
		while( nextNodeId.compare("NULL")!=0 ) {
			HYFLOW_FETCH(nextNodeId, true);
			currentNode = (SkipListNode*)HYFLOW_ON_READ(nextNodeId);
			if(currentNode->value >= givenValue) {
				break;
			}else {
				nextNodeId = currentNode->getNextId(level);
				prevNode = currentNode;
			}
		}
		// For given level prevNode contains just small value
		if(level <= newNode->highestLevel) {
			SkipListNode* pNode = (SkipListNode*)HYFLOW_ON_WRITE(prevNode->getId());
			newNode->setNextId(pNode->getNextId(level), level);
			pNode->setNextId(newNode->getId(), level);
			LOG_DEBUG("SKIPLIST :For level %d node %s inserted between %s and %s\n", level, newNode->getId().c_str(), pNode->getId().c_str(), newNode->getNextId(level).c_str());
		}
	}
	HYFLOW_PUBLISH_OBJECT(newNode);
}

void SkipListNode::addNode(int nodeValue) {
	Atomic<void> atomicfind;
	atomicfind.atomically = SkipListNode::findNodeAtomically;
	atomicfind.execute(NULL, &nodeValue, NULL);
}

void SkipListNode::deleteNodeAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	int givenValue = *((int*)args);

	std::string head="HEAD";
	HYFLOW_FETCH(head, true);

	SkipListNode* prevNode =  (SkipListNode*)HYFLOW_ON_READ(head);
	SkipListNode* currentNode = NULL;
	std::vector<std::string> prevNodes;
	for( int level=0 ; level<=SkipListBenchmark::getSkipListLevels() ; level++ ) {
		prevNodes.push_back("NULL");
	}

	SkipListNode* targetNode = NULL;
	//find objects previous nodes
	for(int level=SkipListBenchmark::getSkipListLevels() ; level>0 ; level-- ) {
		//For given level move until you find the node with just small value
		std::string nextNodeId = prevNode->getNextId(level);
		while( nextNodeId.compare("NULL")!=0 ) {
			HYFLOW_FETCH(nextNodeId, true);
			currentNode = (SkipListNode*)HYFLOW_ON_READ(nextNodeId);
			if(currentNode->value > givenValue) {
				break;
			}else if(currentNode->value == givenValue) {
				targetNode = currentNode;
				LOG_DEBUG("SKIPLIST :At level %d node %s has value %d\n", level, currentNode->getId().c_str(), givenValue);
				break;
			}else {
				nextNodeId = currentNode->getNextId(level);
				prevNode = currentNode;
			}
		}
		prevNodes[level] = prevNode->getId();
	}
	if (targetNode) {
		// Get Writable pointer to targetNode
		targetNode = (SkipListNode*)HYFLOW_ON_WRITE(targetNode->getId());
		for(int level=targetNode->highestLevel; level>0 ; level--) {
			SkipListNode* pvNode = (SkipListNode*)HYFLOW_ON_WRITE(prevNodes[level]);
			pvNode->setNextId(targetNode->getNextId(level), level);
			LOG_DEBUG("SKIPLIST :Set next for %s from %s to %s\n", pvNode->getId().c_str(), targetNode->getId().c_str(), pvNode->getNextId(level).c_str());
		}
		HYFLOW_DELETE_OBJECT(targetNode);
	}
}

void SkipListNode::deleteNode(int nodeValue) {
	Atomic<void> atomicfind;
	atomicfind.atomically = SkipListNode::findNodeAtomically;
	atomicfind.execute(NULL, &nodeValue, NULL);
}

void SkipListNode::findNodeAtomically(HyflowObject* self, void* args, HyflowContext* __context__, void* ignore) {
	int givenValue = *((int*)args);

	std::string head="HEAD";
	HYFLOW_FETCH(head, true);

	SkipListNode* prevNode =  (SkipListNode*)HYFLOW_ON_READ(head);
	SkipListNode* currentNode = NULL;

	bool found=false;
	//find correct place of object
	for(int level=SkipListBenchmark::getSkipListLevels() ; level>0 ; level--) {
		//For given level move until you find the node with just small value
		std::string nextNodeId = prevNode->getNextId(level);
		while( nextNodeId.compare("NULL")!=0 ) {
			HYFLOW_FETCH(nextNodeId, true);
			currentNode = (SkipListNode*)HYFLOW_ON_READ(nextNodeId);
			if(currentNode->value > givenValue) {
				break;
			}else if(currentNode->value == givenValue) {
				found = true;
				LOG_DEBUG("SKIPLIST :Got node %s with value %d\n", currentNode->getId().c_str(), givenValue);
				break;
			}else {
				nextNodeId = currentNode->getNextId(level);
				prevNode = currentNode;
			}
		}
		if (found) {
			break;
		}
	}
}

void SkipListNode::findNode(int nodeValue) {
	Atomic<void> atomicfind;
	atomicfind.atomically = SkipListNode::findNodeAtomically;
	atomicfind.execute(NULL, &nodeValue, NULL);
}

void SkipListNode::addNodeMultiAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore) {
	SkipListArgs* slargs = (SkipListArgs*)args;
	for (int txns = 0; txns < slargs->size ; txns+=1) {
		addNode(slargs->values[txns]);
	}
}

void SkipListNode::deleteNodeMultiAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore) {
	SkipListArgs* slargs = (SkipListArgs*)args;
	for (int txns = 0; txns < slargs->size ; txns+=1) {
		deleteNode(slargs->values[txns]);
	}
}

void SkipListNode::findNodeMultiAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore) {
	SkipListArgs* slargs = (SkipListArgs*)args;
	for (int txns = 0; txns < slargs->size ; txns+=1) {
		findNode(slargs->values[txns]);
	}
}

void SkipListNode::addNodeMulti(int* values, int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("SkipList :Call Add Node with %d in txns %d\n", values[txns], txns);
				addNode(values[txns]);

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		SkipListArgs slargs(values, size);
		Atomic<void> atomicAddMulti;
		atomicAddMulti.atomically = SkipListNode::addNodeMultiAtomically;
		atomicAddMulti.execute(NULL, &slargs, NULL);
	}
}

void SkipListNode::deleteNodeMulti(int* values, int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("SkipList :Call Delete Node with %d in txns %d\n", values[txns], txns);
				deleteNode(values[txns]);

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		SkipListArgs slargs(values, size);
		Atomic<void> atomicDeleteMulti;
		atomicDeleteMulti.atomically = SkipListNode::deleteNodeMultiAtomically;
		atomicDeleteMulti.execute(NULL, &slargs, NULL);
	}
}

void SkipListNode::findNodeMulti(int* values, int size) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=1 ) {
				LOG_DEBUG("SkipList :Call find Node with %d in txns %d\n", values[txns], txns);
				findNode(values[txns]);

				HYFLOW_STORE(&txns, txns);
				HYFLOW_CHECKPOINT_HERE;
			}
		}HYFLOW_ATOMIC_END;
	}else {
		SkipListArgs slargs(values, size);
		Atomic<void> atomicfindMulti;
		atomicfindMulti.atomically = SkipListNode::findNodeMultiAtomically;
		atomicfindMulti.execute(NULL, &slargs, NULL);
	}
}

void SkipListNode::print() {

}

void SkipListNode::getClone(HyflowObject **obj) {
	SkipListNode *ln = new SkipListNode();
	ln->highestLevel = highestLevel;
	ln->value = value;
	for(int level=0; level<=highestLevel ; level++) {
		ln->nextIds.push_back(nextIds[level]);
	}
	this->baseClone(ln);
	*obj = ln;
}

void SkipListNode::test() {
	// create and open a character archive for output
	std::ofstream ofs("skipList", std::ios::out);

	// create class instance
	std::string id="0-0";
	vt_dstm::SkipListNode  l;
	l.setId(id);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << l;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::SkipListNode l1;
	{
		// create and open an archive for input
		std::ifstream ifs("skipList", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> l1;
		// archive and stream closed when destructors are called
		l1.print();
	}
}

} /* namespace vt_dstm */