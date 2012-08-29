/*
 * ObjectTrackerMsg.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include "ObjectTrackerMsg.h"
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"

namespace vt_dstm {

template<class Archive>
void ObjectTrackerMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & objectId;
	ar & nodeId;
	ar & isRead;
}

ObjectTrackerMsg::ObjectTrackerMsg(std::string objId, bool rw) {
	objectId = objId;
	nodeId = -1;
	isRead = rw;
}

ObjectTrackerMsg::ObjectTrackerMsg(std::string objId, bool rw, int id) {
	objectId = objId;
	nodeId = id;
	isRead = rw;
}

ObjectTrackerMsg::~ObjectTrackerMsg() {}

void ObjectTrackerMsg::objectTrackerHandler(HyflowMessage & msg) {
	ObjectTrackerMsg *otmsg = (ObjectTrackerMsg *)msg.getMsg();
	if (otmsg->nodeId == -1) {	// Request Message
		otmsg->nodeId = DirectoryManager::getObjectLocation(otmsg->objectId);
		if (!msg.isCallback) {
			NetworkManager::sendMessage(msg.fromNode,msg);
		}
	} else{
		// Find the wrapper message created for expected response
		HyflowMessageFuture cbfmsg = NetworkManager::getMessageFuture(msg.msg_id, msg.msg_t);
		ObjectAccessMsg oam(otmsg->objectId, otmsg->isRead);
		HyflowMessage hmsg;
		hmsg.setMsg(&oam);
		NetworkManager::sendCallbackMessage(otmsg->nodeId,hmsg, cbfmsg);
	}
}

void ObjectTrackerMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("ObjectTrackerMsgReq", std::ios::out);

	// create class instance
	ObjectTrackerMsg res("3-0", false);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	ObjectTrackerMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("ObjectTrackerMsgReq", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		if (r1.getObjectId().compare("3-0") == 0) {
			std::cout<< "ObjectTrackerMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "ObjectTrackerMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}

} /* namespace vt_dstm */
