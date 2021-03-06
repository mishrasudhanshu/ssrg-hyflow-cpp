/*
 * SkipListNode.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: mishras[at]vt.edu
 */

#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "SkipListNode.h"
#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../BenchmarkExecutor.h"
#include "SkipListBenchmark.h"

#define PROBABILITY 0.5

namespace vt_dstm {

SkipListNode::SkipListNode() {}

SkipListNode::SkipListNode(int val, int counter, int listN) {
	value = val;
	std::stringstream idStr;
	int ownerNode = NetworkManager::getNodeId();
	listNumber = listN;
	idStr<<ownerNode<<"-"<<listN<<"-"<<counter;
	hyId = idStr.str();
	hyVersion = 0;
	highestLevel = getRandomLevel()+1;
	// Level 0 will contain the nodeId itself
	nextIds.push_back(hyId);
	for(int level=1; level<=highestLevel; level++) {
		nextIds.push_back("NULL");
	}
}

SkipListNode::SkipListNode(int val, std::string id, int levels, int listN) {
	value = val;
	hyId = id;
	hyVersion = 0;
	highestLevel = levels;
	nextIds.push_back(hyId);
	listNumber = listN;
	for(int level=1; level<=highestLevel; level++) {
		nextIds.push_back("NULL");
	}
}

int SkipListNode::getRandomLevel(){
	double randomDouble = ((double)(Logger::getCurrentMicroSec()%10000))/10000;
    int level = (int)(log(1.0 - randomDouble) / log(1.0-PROBABILITY));
    LOG_DEBUG("SKIPLIST :NewNode randomDouble=%f and level=%d\n", randomDouble, level);
    return std::min(level, SkipListBenchmark::getSkipListLevels()-1);
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

void SkipListNode::addNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* addReturn) {
	int givenValue = *(((SkipListArgs*)args)->values);

	// Select list to add on
	int listN = (Logger::getCurrentMicroSec()%HYFLOW_SKIPLIST_COUNT) + 1;

	// Redirect Head to correct value
	std::stringstream headStr;
	headStr << "HEAD-"<<listN;
	std::string head = headStr.str();
	HYFLOW_FETCH(head, true);

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create bench name
		std::stringstream benchStr;
		benchStr<<"SK-"<<listN;
		std::string benchName = benchStr.str();

		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<givenValue;
		std::string lockName = absLockStr.str();

		__context__->onLockAccess(benchName, lockName, false);
	}

	SkipListNode* prevNode =  (SkipListNode*)HYFLOW_ON_READ(head);
	SkipListNode* currentNode = NULL;
	SkipListNode* newNode = new SkipListNode(givenValue, SkipListBenchmark::getId(), listN);


	std::vector<std::string> updateNodes;
	//find the correct nodes to update to insert
	const int maxLevels = SkipListBenchmark::getSkipListLevels();
	for(int level=maxLevels; level>0 ; level--) {
		//For given level move until you find the node with just small value
		std::string nextNodeId = prevNode->getNextId(level);
		while( nextNodeId.compare("NULL")!=0 ) {
			HYFLOW_FETCH(nextNodeId, true);
			currentNode = (SkipListNode*)HYFLOW_ON_READ(nextNodeId);
			if(currentNode->value > givenValue) {
				break;
			}else if (currentNode->value == givenValue) {
				// Object is already present return
				LOG_DEBUG("SKIPLIST :Node %s already contain value%d\n", currentNode->hyId.c_str(), givenValue);
				return;
			}else {
				nextNodeId = currentNode->getNextId(level);
				prevNode = currentNode;
			}
		}
		updateNodes.push_back(prevNode->getId());
	}

	LOG_DEBUG("SKIPLIST:Node %s contains %d levels\n", newNode->getId().c_str(), newNode->highestLevel);

	// We have collected all the objects to be updated at different level
	for(int level=maxLevels ; level>0 ; level--) {
		// For given level prevNode contains just small value
		if(level <= newNode->highestLevel) {
			std::string levelNode = updateNodes.at(maxLevels - level);
			SkipListNode* pNode = (SkipListNode*)HYFLOW_ON_WRITE(levelNode);
			newNode->setNextId(pNode->getNextId(level), level);
			pNode->setNextId(newNode->getId(), level);
			LOG_DEBUG("SKIPLIST :For level %d node %s inserted between %s and %s\n", level, newNode->getId().c_str(), pNode->getId().c_str(), newNode->getNextId(level).c_str());
		}
	}

	((SkipListReturn*)addReturn)->success = listN;
	HYFLOW_PUBLISH_OBJECT(newNode);
}

void SkipListNode::addNode(int nodeValue) {
	SkipListArgs sArgs(&nodeValue, 1);
	SkipListReturn sReturn;
	Atomic atomicAdd;

	atomicAdd.atomically = SkipListNode::addNodeAtomically;
	atomicAdd.onAbort = SkipListNode::addAbort;
	atomicAdd.execute(NULL, &sArgs, &sReturn);
}

void SkipListNode::deleteNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	int givenValue = *(((SkipListArgs*)args)->values);
	SkipListReturn* slRt = (SkipListReturn*) rt;

	// Select list to delete from
	int listN = (Logger::getCurrentMicroSec()%HYFLOW_SKIPLIST_COUNT) + 1;

	// Redirect Head to correct value
	std::stringstream headStr;
	headStr << "HEAD-"<<listN;
	std::string head = headStr.str();
	HYFLOW_FETCH(head, true);

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create bench name
		std::stringstream benchStr;
		benchStr<<"SK-"<<listN;
		std::string benchName = benchStr.str();

		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<givenValue;
		std::string lockName = absLockStr.str();

		__context__->onLockAccess(benchName, lockName, false);
	}

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
				slRt->success = listN;
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
	SkipListArgs sArgs(&nodeValue, 1);
	SkipListReturn slRt;
	Atomic atomicDelete;
	atomicDelete.atomically = SkipListNode::deleteNodeAtomically;
	atomicDelete.onAbort = SkipListNode::deleteAbort;
	atomicDelete.execute(NULL, &sArgs, &slRt);
}

void SkipListNode::findNodeAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* ignore) {
	int givenValue = *(((SkipListArgs*)args)->values);

	int listN = 1;
	// Select list to delete from
	listN = (Logger::getCurrentMicroSec()%HYFLOW_SKIPLIST_COUNT) + 1;

	// Redirect Head to correct value
	std::stringstream headStr;
	headStr << "HEAD-"<<listN;
	std::string head = headStr.str();
	HYFLOW_FETCH(head, true);

	if (__context__->getNestingModel() == HYFLOW_NESTING_OPEN ) {
		// Create bench name
		std::stringstream benchStr;
		benchStr<<"SK-"<<listN;
		std::string benchName = benchStr.str();

		// Create unique abstract lock for this transaction
		std::stringstream absLockStr;
		absLockStr<<givenValue;
		std::string lockName = absLockStr.str();

		__context__->onLockAccess(benchName, lockName, true);
	}

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
	SkipListArgs sArgs(&nodeValue, 1);
	Atomic atomicfind;
	atomicfind.atomically = SkipListNode::findNodeAtomically;
	atomicfind.execute(NULL, &sArgs, NULL);
}

void SkipListNode::addNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
	SkipListArgs* slargs = (SkipListArgs*)args;
	for (int txns = 0; txns < slargs->size ; txns+=1) {
		addNode(slargs->values[txns]);
	}
}

void SkipListNode::deleteNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
	SkipListArgs* slargs = (SkipListArgs*)args;
	for (int txns = 0; txns < slargs->size ; txns+=1) {
		deleteNode(slargs->values[txns]);
	}
}

void SkipListNode::findNodeMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore) {
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

				if ((txns+1)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		SkipListArgs slargs(values, size);
		Atomic atomicAddMulti;
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

				if ((txns+1)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		SkipListArgs slargs(values, size);
		Atomic atomicDeleteMulti;
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

				if ((txns+1)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		SkipListArgs slargs(values, size);
		Atomic atomicfindMulti;
		atomicfindMulti.atomically = SkipListNode::findNodeMultiAtomically;
		atomicfindMulti.execute(NULL, &slargs, NULL);
	}
}

void SkipListNode::addAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	int givenValue = *(((SkipListArgs*)args)->values);

	int listN = ((SkipListReturn*)rt)->success;
	if (!listN) {
		return;
	}

	// Redirect Head to correct value
	std::stringstream headStr;
	headStr << "HEAD-"<<listN;
	std::string head = headStr.str();

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

void SkipListNode::deleteAbort(HyflowObject* self, BenchMarkArgs* args, HyflowContext* __context__, BenchMarkReturn* rt) {
	int givenValue = *(((SkipListArgs*)args)->values);

	int listN = ((SkipListReturn*)rt)->success;
	if (!listN) {
		return;
	}

	// Redirect Head to correct value
	std::stringstream headStr;
	headStr << "HEAD-"<<listN;
	std::string head = headStr.str();

	HYFLOW_FETCH(head, false);

	SkipListNode* prevNode =  (SkipListNode*)HYFLOW_ON_READ(head);
	SkipListNode* currentNode = NULL;
	SkipListNode* newNode = new SkipListNode(givenValue, SkipListBenchmark::getId(), listN);

	//find correct place of object to insert
	for(int level=SkipListBenchmark::getSkipListLevels() ; level>0 ; level--) {
		//For given level move until you find the node with just small value
		std::string nextNodeId = prevNode->getNextId(level);
		while( nextNodeId.compare("NULL")!=0 ) {
			HYFLOW_FETCH(nextNodeId, true);
			currentNode = (SkipListNode*)HYFLOW_ON_READ(nextNodeId);
			if(currentNode->value > givenValue) {
				break;
			}else if (currentNode->value == givenValue) {
				return;
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
