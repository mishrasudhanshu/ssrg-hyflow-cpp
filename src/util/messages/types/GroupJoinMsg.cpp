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
#include "GroupJoinMsg.h"
#include "../../networking/NetworkManager.h"

namespace vt_dstm {
template<class Archive>
void GroupJoinMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & nodeId;
	ar & isResponse;
}

GroupJoinMsg::GroupJoinMsg(int id, bool isR) {
	nodeId = id;
	isResponse = isR;
}

GroupJoinMsg::~GroupJoinMsg() {}

void GroupJoinMsg::GroupJoinHandler(HyflowMessage & msg){
	GroupJoinMsg *gmsg = (GroupJoinMsg *)msg.getMsg();
	if (!gmsg->isResponse){
		NetworkManager::atomicIncreaseNodeJoined();
		if (!msg.isCallback) {
			NetworkManager::sendMessage(msg.fromNode, msg);
		}
	}else{
		NetworkManager::setClustered();
	}
}

void GroupJoinMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("groupJoinMsgReq", std::ios::out);

	// create class instance
	vt_dstm::GroupJoinMsg res(15, false);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	vt_dstm::GroupJoinMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("groupJoinMsgReq", std::ios::in);
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

