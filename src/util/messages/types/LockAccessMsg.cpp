/*
 * LockAccessMsg.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "LockAccessMsg.h"
#include "../../networking/NetworkManager.h"
#include "../../../core/context/LockTable.h"
#include "../../messages/MessageMaps.h"
#include "../../logging/Logger.h"
#include "../HyflowMessageFuture.h"

namespace vt_dstm {

LockAccessMsg::LockAccessMsg(std::string objId, int32_t obVer) {
	lock = false;
	locked = false;
	request = false;
	objectId = objId;
	objVersion = obVer;
}

template<class Archive>
void LockAccessMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & lock;
	ar & locked;
	ar & request;
	ar & objectId;
	ar & objVersion;
}

std::string LockAccessMsg::getObjectId() const {
	return objectId;
}

void LockAccessMsg::setObjectId(std::string objectId) {
	this->objectId = objectId;}

void LockAccessMsg::lockAccessHandler(HyflowMessage& m) {
	LockAccessMsg *lmsg = (LockAccessMsg*) (m.getMsg());
	if (lmsg->request) {
		LOG_DEBUG ("Got a Lock request: %s for %s from %d for version%d\n", lmsg->lock?"lock":"unlock", lmsg->objectId.c_str(), m.fromNode, lmsg->objVersion);
		if (lmsg->lock)
			lmsg->locked = LockTable::tryLock(lmsg->objectId, lmsg->objVersion);
		else
			LockTable::tryUnlock(lmsg->objectId,  lmsg->objVersion);

		lmsg->setRequest(false);
		/* If message as callback and network library don't support the callback, we need to reply manually*/
		if (m.isCallback) {
			if (!m.isCallbackSupported) {
				NetworkManager::sendMessage(m.fromNode, m);
			}
		}
	} else {
		LOG_DEBUG ("Got a Lock response: %s for %s\n", lmsg->lock?"lock":"unlock", lmsg->objectId.c_str());
		if (lmsg->lock) {
			HyflowMessageFuture* cbfmsg = MessageMaps::getMessageFuture(m.msg_id,
					m.msg_t);
			if (cbfmsg) {
				cbfmsg->setBoolResponse(lmsg->locked);
				cbfmsg->notifyMessage();
			} else {
				Logger::fatal("Can not find Lock access future for m_id %s\n", m.msg_id.c_str());
			}
		}
		// Unlock response can be ignored
	}
}

LockAccessMsg::~LockAccessMsg() {
}

bool LockAccessMsg::isLock() const {
	return lock;
}

void LockAccessMsg::setLock(bool lock) {
	this->lock = lock;
}

bool LockAccessMsg::isLocked() const {
	return locked;
}

void LockAccessMsg::setLocked(bool locked) {
	this->locked = locked;
}

bool LockAccessMsg::isRequest() const {
	return request;
}

void LockAccessMsg::setRequest(bool request) {
	this->request = request;}

void LockAccessMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("/tmp/ObjectTrackerMsgReq", std::ios::out);

	// create class instance
	LockAccessMsg res("3-0",0);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	LockAccessMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("/tmp/ObjectTrackerMsgReq", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		if (r1.getObjectId().compare("3-0") == 0) {
			std::cout<< "LockAccessMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "LockAccessMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}

} /* namespace vt_dstm */
