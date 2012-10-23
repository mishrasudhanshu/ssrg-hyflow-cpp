/*
 * HyflowMessageFuture.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HyflowMessageFuture.h"
#include "../networking/NetworkManager.h"
#include "../messages/MessageMaps.h"
#include "../messages/MessageHandler.h"
#include "../concurrent/ThreadMeta.h"
#include "../logging/Logger.h"


namespace vt_dstm {

HyflowMessageFuture::HyflowMessageFuture() {
	isReceived = false;
	msg_id = "00";
	dataResponse = NULL;
	forObjectId = "0-0";
}

HyflowMessageFuture::~HyflowMessageFuture() {
	if (msg_t != MSG_TYPE_INVALID) {
		try {
			MessageMaps::removeMessageFuture(msg_id, msg_t);
		} catch(std::string & s) {
			Logger::fatal("%s\n",s.c_str());
			throw s;
		}
//		if (dataResponse) {
//			HyflowObject* saveData = dataResponse;
//			dataResponse = NULL;
//			// TODO: cleanup: Object is deleted on context destructor call
//			delete saveData;
//		}
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
	     LOG_DEBUG("HMF :Set received for m_id %s\n", msg_id.c_str());
	 }
	 onReceive.notify_all();
}

void HyflowMessageFuture::setId(const std::string & id){
	msg_id = id;
}

const std::string & HyflowMessageFuture::getId() {
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

void HyflowMessageFuture::createIdNRegisterFuture(){
	std::stringstream idNameStr;
	idNameStr << getCurrentTime()<< "|"<<msg_t<<"|"<<NetworkManager::getNodeId()<<"|"<<ThreadMeta::getThreadId()<<"|"<<forObjectId;
	msg_id = idNameStr.str();
	MessageHandler::registerMessageFuture(msg_id, msg_t, *this);
}

unsigned long long HyflowMessageFuture::getCurrentTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	// Hopefully Transaction will not take more than 290 hrs
	return (tv.tv_sec%10000000)*10000 + 0.01*tv.tv_usec;		// 14 Digits
}

} /* namespace vt_dstm */
