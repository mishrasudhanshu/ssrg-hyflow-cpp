/*
 * AbstractLockMsg.cpp
 *
 *  Created on: Dec 15, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "AbstractLockMsg.h"
#include "../../logging/Logger.h"
#include "../../../core/context/AbstractLockTable.h"

#include "../../networking/NetworkManager.h"
#include "../../messages/MessageMaps.h"
#include "../HyflowMessageFuture.h"

namespace vt_dstm {

AbstractLockMsg::AbstractLockMsg(AbstractLock* absLock, bool lock, bool rd) {
	doLock = lock;
	request = true;
	abstractLock = absLock;
	read = rd;
	response = false;
}

AbstractLockMsg::~AbstractLockMsg() {}

template<class Archive>
void AbstractLockMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);

	ar & doLock;
	ar & request;
	ar & read;
	ar & abstractLock;
	ar & response;
}

void AbstractLockMsg::absLockAccessHandler(HyflowMessage& m) {
	AbstractLockMsg *almsg = (AbstractLockMsg*) (m.getMsg());
	if (almsg->request) {
		if (almsg->doLock) {
			LOG_DEBUG("ABL :Got Lock  request for %s\n", almsg->abstractLock->getLockName().c_str());
			bool resp = AbstractLockTable::tryLock(almsg->abstractLock, almsg->read);
			almsg->response = resp;
		}else {
			LOG_DEBUG("ABL :Got unlock  request for %s\n", almsg->abstractLock->getLockName().c_str());
			AbstractLockTable::unlock(almsg->abstractLock, almsg->read);
		}
		almsg->request = false;
		/* If message as callback and network library don't support the callback, we need to reply manually*/
		if (m.isCallback) {
			if (!m.isCallbackSupported) {
				NetworkManager::sendMessage(m.fromNode, m);
			}
		}
	}else {
		if (almsg->doLock) {
			// Find the MessageFuture created for expected response
			HyflowMessageFuture* cbfmsg = MessageMaps::getMessageFuture(m.msg_id,
					m.msg_t);
			if (cbfmsg) {
				LOG_DEBUG("ABL :Got Lock  response %s for %s\n", almsg->abstractLock->getLockName().c_str(), almsg->response?"true":"false");
				cbfmsg->setBoolResponse(almsg->response);
			}else {
				Logger::fatal("Can not find Object access future for m_id %s\n", m.msg_id.c_str());
			}
		}
		// Unlock response can be ignored
	}
}

// Serialisation Test of object
void AbstractLockMsg::serializationTest() {
	{
		// create and open a character archive for output
		std::ofstream ofs("/tmp/AbstractLockMsg", std::ios::out);

		// create class instance
		AbstractLock absLock("Test", "Test-0", 0);
		AbstractLockMsg res(&absLock, 0, true);

		// save data to archive
		{
			boost::archive::text_oarchive oa(ofs);
			// write class instance to archive
			oa << res;
			// archive and stream closed when destructors are called
		}

		// ... some time later restore the class instance to its original state
		AbstractLockMsg r1;
		{
			// create and open an archive for input
			std::ifstream ifs("/tmp/AbstractLockMsg", std::ios::in);
			boost::archive::text_iarchive ia(ifs);
			// read class state from archive
			ia >> r1;
			// archive and stream closed when destructors are called
			AbstractLock *absLock = r1.abstractLock;
			if (absLock->getLockName().compare("Test-0") == 0) {
				std::cout<< "AbstractLockMsg serialization Test passed"<<std::endl;
			}else {
				std::cerr<< "AbstractLockMsg serialization value="<<absLock->getLockName()<<"Test FAILED!!!"<<std::endl;
			}
		}
	}
}

} /* namespace vt_dstm */
