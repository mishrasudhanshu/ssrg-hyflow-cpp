/*
 * HyflowMessageFuture.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HyflowMessageFuture.h"

namespace vt_dstm {

HyflowMessageFuture::HyflowMessageFuture() {isReceived = false;}
HyflowMessageFuture::~HyflowMessageFuture() {
	NetworkManager::removeMessageFuture(msg_id, msg_t);
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
	     boost::lock_guard<boost::mutex> lock(msgMutex);
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

HyMessageType HyflowMessageFuture::getType(){
	return msg_t;
}

} /* namespace vt_dstm */
