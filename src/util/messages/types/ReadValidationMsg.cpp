/*
 * ReadValidationMsg.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "ReadValidationMsg.h"
#include "../HyflowMessageFuture.h"
#include "../HyflowMessage.h"
#include "../../networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/context/LockTable.h"
#include "../../messages/MessageMaps.h"

namespace vt_dstm {

ReadValidationMsg::ReadValidationMsg() {
	validationResponse = false;
	request = false;
}

ReadValidationMsg::ReadValidationMsg(std::string objId, int32_t obVer) {
	validationResponse = false;
	request = false;
	objectId = objId;
	objectVersion = obVer;
}

std::string ReadValidationMsg::getObjectId() const {
	return objectId;
}

void ReadValidationMsg::setObjectId(std::string objectId) {
	this->objectId = objectId;
}

bool ReadValidationMsg::isRequest() const {
	return request;
}

void ReadValidationMsg::setRequest(bool request) {
	this->request = request;}

void ReadValidationMsg::readValidationHandle(HyflowMessage & msg) {
	ReadValidationMsg* rvmsg = (ReadValidationMsg*) msg.getMsg();
	if (rvmsg->request) {
		HyflowObject* rObj = DirectoryManager::getObjectLocally(rvmsg->objectId,true);
		// If object is locked it means object might have been moved to some other owner
		if ((rObj->getVersion() == rvmsg->objectVersion) && !LockTable::isLocked(rvmsg->objectId, 0)) {
			rvmsg->validationResponse = true;
		} else {
			rvmsg->validationResponse = false;
		}
	}else {
		HyflowMessageFuture & cbfmsg = MessageMaps::getMessageFuture(msg.msg_id,
						msg.msg_t);
		cbfmsg.setBoolResponse(rvmsg->validationResponse);
		cbfmsg.notifyMessage();
	}
}

ReadValidationMsg::~ReadValidationMsg() {}

template<class Archive>
void ReadValidationMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & validationResponse;
	ar & objectId;
	ar & objectVersion;
}

void ReadValidationMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("/tmp/ObjectTrackerMsgReq", std::ios::out);

	// create class instance
	ReadValidationMsg res("3-0",5);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	ReadValidationMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("/tmp/ObjectTrackerMsgReq", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		if (r1.getObjectId().compare("3-0") == 0) {
			std::cout<< "ReadValidationMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "ReadValidationMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}

} /* namespace vt_dstm */
