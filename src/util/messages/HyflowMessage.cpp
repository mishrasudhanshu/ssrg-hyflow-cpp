/*
 * HyflowMessage.cpp
 *  Created on: Aug 23, 2012
 *      Author: mishras[at]vt.edu
 */
#include <cstdlib>
#include <string>
#include <cstdio>
#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "HyflowMessage.h"
#include "../networking/NetworkManager.h"
#include "../messages/MessageMaps.h"
#include "types/ObjectAccessMsg.h"
#include "types/SynchronizeMsg.h"
#include "types/ObjectTrackerMsg.h"
#include "types/RegisterObjectMsg.h"
#include "types/LockAccessMsg.h"
#include "types/ReadValidationMsg.h"
#include "types/DummyTestMsg.h"
#include "types/AbstractLockMsg.h"
#include "../../core/context/ContextManager.h"

namespace vt_dstm{

bool HyflowMessage::isMessageInit = false;
/*
 * By Default each message send is a callback, and we assume the
 * callback is supported in the library. if no reply required
 * specify explicitly
 */
HyflowMessage::HyflowMessage(){
	isCallback = true; isCallbackSupported = true; msg_id = "00";
	fromNodeClock = ContextManager::getClock();
	forObjectId = "0-0";
	msg = NULL;
}

/*
 * Use this constructor when any message related to a object is created
 * ForObjId is defined to create a unique Id for the message future
 * As in few case all the values can be same expect forObjectId
 */
HyflowMessage::HyflowMessage(const std::string & forObjId){
	isCallback = true; isCallbackSupported = true; msg_id = "00";
	fromNodeClock = ContextManager::getClock();
	forObjectId = forObjId;
	msg = NULL;
}

void HyflowMessage::registerMessageHandlers()	{
	if (!isMessageInit) {
		MessageMaps::MessageMapsInit();
		MessageMaps::registerHandler(MSG_GRP_SYNC, &SynchronizeMsg::synchronizeHandler);
		MessageMaps::registerHandler(MSG_TRK_OBJECT, &ObjectTrackerMsg::objectTrackerHandler);
		MessageMaps::registerHandler(MSG_ACCESS_OBJECT, &ObjectAccessMsg::objectAccessHandler);
		MessageMaps::registerHandler(MSG_REGISTER_OBJ, &RegisterObjectMsg::registerObjectHandler);
		MessageMaps::registerHandler(MSG_LOCK_ACCESS, &LockAccessMsg::lockAccessHandler);
		MessageMaps::registerHandler(MSG_READ_VALIDATE, &ReadValidationMsg::readValidationHandle);
		MessageMaps::registerHandler(MSG_TYPE_DUMMY, &DummyTestMsg::DummyTestMsgHandler);
		MessageMaps::registerHandler(MSG_ABSTRACT_LOCK, &AbstractLockMsg::absLockAccessHandler);
		isMessageInit = true;
	}
}

template<class Archive>
void HyflowMessage::registerMessageTypes(Archive & ar){
	ar.register_type(static_cast<ObjectAccessMsg*>(NULL));
	ar.register_type(static_cast<SynchronizeMsg*>(NULL));
	ar.register_type(static_cast<ObjectTrackerMsg*>(NULL));
	ar.register_type(static_cast<RegisterObjectMsg*>(NULL));
	ar.register_type(static_cast<LockAccessMsg*>(NULL));
	ar.register_type(static_cast<ReadValidationMsg*>(NULL));
	ar.register_type(static_cast<DummyTestMsg*>(NULL));
	ar.register_type(static_cast<AbstractLockMsg*>(NULL));
}

void  HyflowMessage::syncClocks()	{
	int localClock = ContextManager::getClock();
	while (localClock < fromNodeClock) {
		if (ContextManager::atomicUpdateClock(fromNodeClock, localClock))
			return;
		localClock = ContextManager::getClock();
	}
}

void HyflowMessage::init(HyMessageType msg_t, bool isCallback) {
	this->msg_t = msg_t;
	this->isCallback = isCallback;
}

std::string HyflowMessage::getForObjectId() const {
	return forObjectId;
}

void HyflowMessage::setForObjectId(std::string forObjectId) {
	this->forObjectId = forObjectId;
}

void HyflowMessage::test() {
	//First test all different message serialization
	std::cout << "\n---Testing Serialization---\n" << std::endl;
	SynchronizeMsg gjmsg;
	gjmsg.serializationTest();
	ObjectAccessMsg oamsg;
	oamsg.serializationTest();
	ObjectTrackerMsg otmsg;
	otmsg.serializationTest();
	RegisterObjectMsg romsg;
	romsg.serializationTest();
	LockAccessMsg lamsg;
	lamsg.serializationTest();
	ReadValidationMsg rvmsg;
	rvmsg.serializationTest();
	AbstractLockMsg absmsg;
	absmsg.serializationTest();
	// create and open a character archive for output
	std::ofstream ofs("/tmp/HyflowMessage", std::ios::out);
	// create class instance
	vt_dstm::ObjectAccessMsg bq("3-0", true);
	vt_dstm::HyflowMessage res;
	res.msg_t = vt_dstm::MSG_ACCESS_OBJECT;
	res.setMsg(&bq);
	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}
	// ... some time later restore the class instance to its original state
	vt_dstm::HyflowMessage r1;
	{
		// create and open an archive for input
		std::ifstream ifs("/tmp/HyflowMessage", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		vt_dstm::ObjectAccessMsg* bq1 =
				(vt_dstm::ObjectAccessMsg*) (r1.getMsg());
		if (bq1->getId().compare("3-0") == 0) {
			std::cout<< "HyflowMessage serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "HyflowMessage serialization Test FAILED!!!"<<std::endl;
		}
	}
}

}	/*namespace vt_dstm*/

