/*
 * GroupJoinMsg.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include "SynchronizeMsg.h"
#include "../../networking/NetworkManager.h"
#include "../../logging/Logger.h"

namespace vt_dstm {

template<class Archive>
void SynchronizeMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & nodeId;
	ar & isResponse;
	ar & requestNo;
}

SynchronizeMsg::SynchronizeMsg(int id, bool isR, int rn) {
	nodeId = id;
	isResponse = isR;
	requestNo = rn;
}

SynchronizeMsg::~SynchronizeMsg() {}

void SynchronizeMsg::synchronizeHandler(HyflowMessage & msg){
	SynchronizeMsg *synmsg = (SynchronizeMsg *)msg.getMsg();
	if (!synmsg->isResponse){	// Node 0 will receive this message
		Logger::debug("SYNC_MSG: Got Synchronize request message\n");
		if(NetworkManager::allNodeJoined(synmsg->requestNo))	// If you are last message set synchronized
			NetworkManager::setSynchronized(synmsg->requestNo);
		else
			NetworkManager::waitTillSynchronized(synmsg->requestNo);

		synmsg->isResponse = true;
		if (!msg.isCallback) {
			synmsg->isResponse = true;
			NetworkManager::sendMessage(msg.fromNode, msg);
		}
	}else{
		Logger::debug("SYNC_MSG: Got Synchronize response message\n");
		if( NetworkManager::getNodeId() != 0)	// Node 0 is already awaken
			NetworkManager::setSynchronized(synmsg->requestNo);
	}
}

void SynchronizeMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("/tmp/groupJoinMsgReq", std::ios::out);

	// create class instance
	SynchronizeMsg res(15, false, 1);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	SynchronizeMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("/tmp/groupJoinMsgReq", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		if (r1.getNodeId() == 15) {
			std::cout<< "GroupJoinMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "GroupJoinMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}
} /* namespace vt_dstm */

