/*
 * ObjectTrackerMsg.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "ObjectTrackerMsg.h"
#include "ObjectAccessMsg.h"
#include "../../networking/NetworkManager.h"
#include "../../messages/MessageMaps.h"
#include "../../logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/context/ContextManager.h"

namespace vt_dstm {

template<class Archive>
void ObjectTrackerMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & objectId;
	ar & owner;
	ar & isRead;
}

ObjectTrackerMsg::ObjectTrackerMsg(std::string objId, bool rw) {
	objectId = objId;
	owner = -1;
	isRead = rw;
}

ObjectTrackerMsg::ObjectTrackerMsg(std::string objId, bool rw, int id) {
	objectId = objId;
	owner = id;
	isRead = rw;
}

ObjectTrackerMsg::~ObjectTrackerMsg() {}

void ObjectTrackerMsg::objectTrackerHandler(HyflowMessage & msg) {
	ObjectTrackerMsg *otmsg = (ObjectTrackerMsg *)msg.getMsg();
	/*
	 * Object read is serialized at tracker value read
	 */
	if (otmsg->owner == -1) {	// Request Message
		otmsg->owner = DirectoryManager::getObjectLocation(otmsg->objectId);
		LOG_DEBUG("Got object Tracker request from %d replied for %s with owner %d\n", msg.fromNode, otmsg->objectId.c_str(), otmsg->owner);

		if (msg.isCallback) {
			if (!msg.isCallbackSupported) {
				NetworkManager::sendMessage(msg.fromNode,msg);
			}
		}
	} else{

		// LESSON: Make sure no copy constructor is called on HyflowMessageFuture!!!
		HyflowMessageFuture* cbfmsg = MessageMaps::getMessageFuture(msg.msg_id, msg.msg_t);
		if (cbfmsg) {
			// After getting a reference remove for map
			MessageMaps::removeMessageFuture(msg.msg_id, msg.msg_t);
			// So at destructor don't try to remove itself if its type not overridden
			cbfmsg->setType(MSG_TYPE_DUMMY);

			int myNode = NetworkManager::getNodeId();
			if (myNode == otmsg->owner) {
				LOG_DEBUG("Object Tracker Response: Local object %s\n", otmsg->objectId.c_str());

				// Update sender clock for requesting context
				HyflowContext *c = ContextManager::findContext(cbfmsg->getTxnId());
				// Transaction forwarding will be done on before access call, currently just note
				// down the highest sender clock.
				c->updateClock(msg.fromNodeClock);

				HyflowObject* obj = DirectoryManager::getObjectLocally(otmsg->objectId, otmsg->isRead);
				cbfmsg->setDataResponse(obj);
				cbfmsg->notifyMessage();
			} else if (otmsg->owner == -1) {
				LOG_DEBUG("Object Tracker object %s got deleted\n", otmsg->objectId.c_str());
				cbfmsg->setDataResponse(NULL);
				cbfmsg->notifyMessage();
			}else {
				const std::string & objId = otmsg->objectId;
				ObjectAccessMsg oam(objId, otmsg->isRead);
				HyflowMessage hmsg(objId);
				hmsg.msg_t = MSG_ACCESS_OBJECT;
				hmsg.isCallback = true;
				hmsg.setMsg(&oam);
				LOG_DEBUG("Object Tracker Response: send request to %d for %s\n", otmsg->owner,otmsg->objectId.c_str());
				NetworkManager::sendCallbackMessage(otmsg->owner,hmsg, *cbfmsg);
			}
		}else {
			Logger::fatal("Can not find Tracker object future for m_id %s\n", msg.msg_id.c_str());
		}
	}
}

void ObjectTrackerMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("/tmp/ObjectTrackerMsgReq", std::ios::out);

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
		std::ifstream ifs("/tmp/ObjectTrackerMsgReq", std::ios::in);
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
