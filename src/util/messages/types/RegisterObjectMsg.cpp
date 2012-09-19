/*
 * RegisterObjectMsg.cpp
 *
 *  Created on: Aug 28, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "RegisterObjectMsg.h"
#include "../../../benchMarks/BenchmarkExecutor.h"
#include "../../networking/NetworkManager.h"
#include "../../messages/MessageMaps.h"
#include "../../logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"

namespace vt_dstm {

RegisterObjectMsg::RegisterObjectMsg(std::string id, unsigned long long tid) {
	objectId = id;
	owner = -1;
	txnId = tid;
	request = true;
}

RegisterObjectMsg::RegisterObjectMsg(std::string id, int ow, unsigned long long tid) {
	objectId = id;
	owner = ow;
	txnId = tid;
	request = true;
}

RegisterObjectMsg::~RegisterObjectMsg() {}

template<class Archive>
void RegisterObjectMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & txnId;
	ar & owner;
	ar & objectId;
	ar & request;
}

// FIXME: In List might require forwarding call
void RegisterObjectMsg::registerObjectHandler(HyflowMessage & msg) {
	RegisterObjectMsg* romsg = (RegisterObjectMsg*) (msg.getMsg());
	if (romsg->request) {
		LOG_DEBUG("Got Register Object Request\n");
		if (romsg->owner != -1)
			DirectoryManager::registerObjectLocally(romsg->objectId, romsg->owner,
					romsg->txnId);
		else
			DirectoryManager::unregisterObjectLocally(romsg->objectId,
					romsg->txnId);
		romsg->request = false;
	} else {
		LOG_DEBUG("Got Register Object Response\n");
		HyflowMessageFuture & cbfmsg = MessageMaps::getMessageFuture(msg.msg_id,
						msg.msg_t);
		cbfmsg.notifyMessage();
	}
}
bool RegisterObjectMsg::isRequest() const {
	return request;
}

void RegisterObjectMsg::setRequest(bool request) {
	this->request = request;
}

void RegisterObjectMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("/tmp/RegisterObjectMsg", std::ios::out);

	// create class instance
	RegisterObjectMsg res("1-1", 0);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	RegisterObjectMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("/tmp/RegisterObjectMsg", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		if (r1.objectId.compare("1-1") == 0) {
			std::cout<< "RegisterObjectMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "RegisterObjectMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}

} /* namespace vt_dstm */
