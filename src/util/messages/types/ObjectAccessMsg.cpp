/*
 * ObjectRequestMsg.cpp
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include "ObjectAccessMsg.h"
#include "../../../benchMarks/Benchmark.h"
#include "../../networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"

#include "../../../benchMarks/tm/bank/BankAccount.h"

namespace vt_dstm {

ObjectAccessMsg::ObjectAccessMsg(std::string Id, bool rw) {
	id = Id;
	isRead = rw;
	object = NULL;
}

template<class Archive>
void ObjectAccessMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	// Register object pointers
	Benchmark::registerObjectTypes(ar);
	ar & object;
	ar & id;
	ar & isRead;
}

void ObjectAccessMsg::print() {
	std::cout << " Id " << id << " isRead " << isRead;
}

void ObjectAccessMsg::objectAccessHandler(HyflowMessage & msg) {
	ObjectAccessMsg *oamsg = (ObjectAccessMsg *) msg.getMsg();
	if (!oamsg->object) {
		HyflowObject & obj = DirectoryManager::getObjectLocally(oamsg->id,
				oamsg->isRead);
		oamsg->object = &obj;
		if (!msg.isCallback) {
			NetworkManager::sendMessage(msg.fromNode, msg);
		}
	} else {
		// Find the MessageFuture created for expected response
		HyflowMessageFuture cbfmsg = NetworkManager::getMessageFuture(msg.msg_id,
				msg.msg_t);
		HyflowObject *obj = NULL;
		oamsg->object->getClone(obj);
		DirectoryManager::updateObjectLocally(*obj);
		cbfmsg.notifyMessage();
	}
}

// Serialisation Test of object
void ObjectAccessMsg::serializationTest() {
	{
		// create and open a character archive for output
		std::ofstream ofs("objectAccessReq", std::ios::out);

		// create class instance
		ObjectAccessMsg res("0-0", true);

		// save data to archive
		{
			boost::archive::text_oarchive oa(ofs);
			// write class instance to archive
			oa << res;
			// archive and stream closed when destructors are called
		}

		// ... some time later restore the class instance to its original state
		ObjectAccessMsg r1;
		{
			// create and open an archive for input
			std::ifstream ifs("objectAccessReq", std::ios::in);
			boost::archive::text_iarchive ia(ifs);
			// read class state from archive
			ia >> r1;
			// archive and stream closed when destructors are called
			if (r1.getId().compare("0-0") == 0) {
				std::cout<< "ObjectAccessRequest serialization Test passed"<<std::endl;
			}else {
				std::cerr<< "ObjectAccessRequest serialization Test FAILED!!!"<<std::endl;
			}
		}
	}

	{
		// create and open a character archive for output
		std::ofstream ofs("objectAccessRes", std::ios::out);

		BankAccount bac(1000, "3-0");
		// create class instance
		ObjectAccessMsg res;
		res.setObject(&bac);

		HyflowMessage msg;
		msg.setMsg(&res);

		// save data to archive
		{
			boost::archive::text_oarchive oa(ofs);
			// write class instance to archive
			oa << msg;
			// archive and stream closed when destructors are called
		}

		// ... some time later restore the class instance to its orginal state
		ObjectAccessMsg* r1;
		BankAccount *b1;
		HyflowMessage msg1;
		{
			// create and open an archive for input
			std::ifstream ifs("objectAccessRes", std::ios::in);
			boost::archive::text_iarchive ia(ifs);
			// read class state from archive
			ia >> msg1;
			// archive and stream closed when destructors are called
			r1 = (vt_dstm::ObjectAccessMsg*) msg1.getMsg();
			b1 = (vt_dstm::BankAccount*) r1->getObject();
			if (b1->getId().compare("3-0") == 0) {
				std::cout<< "ObjectAccessResponse serialization Test passed"<<std::endl;
			}else {
				std::cerr<< "ObjectAccessResponse serialization Test FAILED!!!"<<std::endl;
			}
		}
	}
}

} /* namespace vt_dstm */

