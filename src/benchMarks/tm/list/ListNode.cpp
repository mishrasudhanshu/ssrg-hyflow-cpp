/*
 * ListNode.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include "ListNode.h"
#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"


namespace vt_dstm {

ListNode::ListNode() {}

template<class Archive>
void ListNode::serialize(Archive & ar, const unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & nextId;
    	ar & value;
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

void ListNode::addNode(int value, HyflowContext *c, HyflowObjectFuture & fu) {
	std::string head="HEAD";
	DirectoryManager::locateAsync(head, true, c->getTxnId(), fu);
}

void ListNode::addNode(int value) {
	bool commit = true;
	for (int i = 0; i < 0x7fffffff; i++) {
		HyflowContext* c = ContextManager::getInstance();
		HyflowObjectFuture of1("HEAD", true, c->getTxnId());
		try {
			addNode(value, c, of1);
		} catch (TransactionException & ex) {
			ex.print();
			commit = false;
		} catch (...) {
			throw;
		}
		if (commit) {
			try {
				c->commit();
				LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");
			} catch (TransactionException & ex) {
				ex.print();
				continue;
			} catch(...) {
				throw;
			}
			return;
		}
	}
	throw new TransactionException("Failed to commit the transaction in the defined retries.");
}

void ListNode::deleteNode(int value, HyflowContext *c) {

}

void ListNode::deleteNode(int value) {

}

void ListNode::sumNodes(HyflowContext *c) {

}

void ListNode::sumNodes() {

}

void ListNode::findNode(int value, HyflowContext *c) {

}

void ListNode::findNode(int value) {

}

void ListNode::print() {

}

void ListNode::getClone(HyflowObject **obj) {

}

} /* namespace vt_dstm */

