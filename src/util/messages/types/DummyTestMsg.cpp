/*
 * DummyTestMsg.cpp
 *
 *  Created on: Nov 10, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include "DummyTestMsg.h"
#include "../../logging/Logger.h"
#include "../HyflowMessageFuture.h"
#include "../MessageMaps.h"

namespace vt_dstm {

DummyTestMsg::DummyTestMsg() {}

DummyTestMsg::DummyTestMsg(std::string m) {
	request = true;
	msg = m;
}

DummyTestMsg::~DummyTestMsg() {}

template<class Archive>
void DummyTestMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	ar & request;
	ar & msg;
}

void DummyTestMsg::DummyTestMsgHandler(HyflowMessage& m) {
	DummyTestMsg *dmsg = (DummyTestMsg*) (((m.getMsg())));
	if (dmsg->request) {
		LOG_DEBUG("DT :Got Message :%s\n", dmsg->msg.c_str());
		dmsg->request = false;
	}else {
		LOG_DEBUG("DT :Got Message Response\n");
		HyflowMessageFuture* cbfmsg = MessageMaps::getMessageFuture(m.msg_id,
				m.msg_t);
		LOG_DEBUG("DT :Got Message :%s\n", dmsg->msg.c_str());
		if (cbfmsg) {
			cbfmsg->notifyMessage();
		} else {
			Logger::fatal("Can not find dummy future for m_id %s\n",
					m.msg_id.c_str());
		}
	}
}

std::string DummyTestMsg::getMsg() const {
	return msg;
}

bool DummyTestMsg::isRequest() const {
	return request;
}

void DummyTestMsg::setRequest(bool request) {
	this->request = request;
}

void DummyTestMsg::setMsg(std::string msg) {
	this->msg = msg;
}

void DummyTestMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("/tmp/DummyTestMsg", std::ios::out);

	// create class instance
	DummyTestMsg res("3-0");

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	DummyTestMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("/tmp/DummyTestMsg", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		if (r1.getMsg().compare("3-0") == 0) {
			std::cout<< "DummyTestMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "DummyTestMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}

} /* namespace vt_dstm */
