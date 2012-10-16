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
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
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
	ar & myIP;
	ar & clusterIPs;
}

SynchronizeMsg::SynchronizeMsg(int id, bool isR, int rn) {
	if (rn == 1) {	// If first ever request pass IPs
		if (isR) {
			std::map<int, std::string> & ipMap = NetworkManager::getIpMap();
			clusterIPs.insert(ipMap.begin(), ipMap.end());
		}else {
			myIP = NetworkManager::getNodeIP();
		}
	}
	nodeId = id;
	isResponse = isR;
	requestNo = rn;
}

SynchronizeMsg::~SynchronizeMsg() {}

void SynchronizeMsg::synchronizeHandler(HyflowMessage & msg){
	SynchronizeMsg *synmsg = (SynchronizeMsg *)msg.getMsg();
	if (!synmsg->isResponse){	// Node 0 will receive this message
		LOG_DEBUG("SYNC_MSG: Got Synchronize request message from %d\n", msg.fromNode);
		if (synmsg->requestNo == 1) {
			NetworkManager::registerNode(synmsg->nodeId, synmsg->myIP);
		}
		if(NetworkManager::allNodeJoined(synmsg->requestNo))	{// If you are last message set synchronized
			LOG_DEBUG("SYNC_MSG: Last message from %d\n", msg.fromNode);
			NetworkManager::replySynchronized(synmsg->requestNo);
		}
		/*This message is always send one way */
	}else{
		LOG_DEBUG("SYNC_MSG: Got Synchronize response message\n");
		if ((synmsg->requestNo == 1) && (NetworkManager::getNodeId()!=0)) {
			NetworkManager::registerCluster(synmsg->clusterIPs);
		}
		NetworkManager::notifyCluster(synmsg->requestNo);
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

