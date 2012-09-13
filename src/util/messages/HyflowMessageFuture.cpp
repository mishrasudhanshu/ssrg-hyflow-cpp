/*
 * HyflowMessageFuture.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HyflowMessageFuture.h"
#include "../networking/NetworkManager.h"

namespace vt_dstm {

HyflowMessageFuture::HyflowMessageFuture() {isReceived = false; msg_id=0; dataResponse = NULL;}
HyflowMessageFuture::~HyflowMessageFuture() {
	if (msg_t != MSG_TYPE_INVALID) {
		NetworkManager::removeMessageFuture(msg_id, msg_t);
		delete dataResponse;
	}
}

void HyflowMessageFuture::waitOnFuture(){
	boost::unique_lock<boost::mutex> lock(msgMutex);
	while (!isReceived) {
		onReceive.wait(lock);
	}
}

bool HyflowMessageFuture::isComplete() {
	return isReceived;
}

void HyflowMessageFuture::notifyMessage() {
	{
	     boost::unique_lock<boost::mutex> lock(msgMutex);
	     isReceived=true;
	 }
	 onReceive.notify_all();
}

void HyflowMessageFuture::setId(unsigned long long id){
	msg_id = id;
}

unsigned long long HyflowMessageFuture::getId() {
	return msg_id;
}

void HyflowMessageFuture::setType(HyMessageType t) {
	msg_t = t;
}

bool HyflowMessageFuture::isBoolResponse() const {
	return boolResponse;
}

void HyflowMessageFuture::setBoolResponse(bool boolResponse) {
	this->boolResponse = boolResponse;
}

HyflowObject* HyflowMessageFuture::getDataResponse() const {
	return dataResponse;
}

void HyflowMessageFuture::setDataResponse(HyflowObject* dataResponse) {
	this->dataResponse = dataResponse;
}

int HyflowMessageFuture::getIntResponse() const {
	return intResponse;
}

void HyflowMessageFuture::setIntResponse(int intResponse) {
	this->intResponse = intResponse;
}

std::string HyflowMessageFuture::getStringResponse() const {
	return stringResponse;
}

unsigned long long HyflowMessageFuture::getTxnId() const {
	return txnId;
}

void HyflowMessageFuture::setTxnId(unsigned long long txnId) {
	this->txnId = txnId;}

void HyflowMessageFuture::setStringResponse(std::string stringResponse) {
	this->stringResponse = stringResponse;}

HyMessageType HyflowMessageFuture::getType(){
	return msg_t;
}

} /* namespace vt_dstm */
