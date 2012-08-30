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
#include "../../../benchMarks/Benchmark.h"
#include "../../networking/NetworkManager.h"
#include "../../../core/directory/DirectoryManager.h"

namespace vt_dstm {

RegisterObjectMsg::RegisterObjectMsg(HyflowObject *obj, unsigned long long tid) {
	object = obj;
	txnId = tid;
	objectId = obj->getId();
}

RegisterObjectMsg::RegisterObjectMsg(std::string id, unsigned long long tid) {
	object = NULL;
	txnId = tid;
	objectId = id;
}

RegisterObjectMsg::~RegisterObjectMsg() {}

template<class Archive>
void RegisterObjectMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<BaseMessage>(*this);
	// Register object pointers
	Benchmark::registerObjectTypes(ar);
	ar & object;
	ar & txnId;
	ar & objectId;
}

void RegisterObjectMsg::registerObjectHandler(HyflowMessage & msg) {
	RegisterObjectMsg* romsg = (RegisterObjectMsg*)msg.getMsg();
	if (romsg->object)
		DirectoryManager::registerObjectLocally(*romsg->object, romsg->txnId);
	else
		DirectoryManager::unregisterObjectLocally(romsg->objectId, romsg->txnId);
}

HyflowObject* RegisterObjectMsg::getObject(){
	return object;
}
void RegisterObjectMsg::serializationTest(){
	// create and open a character archive for output
	std::ofstream ofs("RegisterObjectMsg", std::ios::out);

	// create class instance
	BankAccount ba(1000, "1-0");
	RegisterObjectMsg res(&ba, 0);

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
		std::ifstream ifs("RegisterObjectMsg", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		BankAccount *ba1 = (BankAccount *)r1.getObject();
		if (ba1->getId().compare("1-0") == 0) {
			std::cout<< "RegisterObjectMsg serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "RegisterObjectMsg serialization Test FAILED!!!"<<std::endl;
		}
	}
}

} /* namespace vt_dstm */
