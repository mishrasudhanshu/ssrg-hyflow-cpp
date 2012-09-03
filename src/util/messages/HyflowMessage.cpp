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
#include "types/ObjectAccessMsg.h"
#include "types/SynchronizeMsg.h"
#include "types/ObjectTrackerMsg.h"
#include "types/RegisterObjectMsg.h"

namespace vt_dstm{

void HyflowMessage::registerMessageHandlers()	{
	NetworkManager::registerHandler(MSG_GRP_SYNC, &SynchronizeMsg::synchronizeHandler);
	NetworkManager::registerHandler(MSG_TRK_OBJECT, &ObjectTrackerMsg::objectTrackerHandler);
	NetworkManager::registerHandler(MSG_ACCESS_OBJECT, &ObjectAccessMsg::objectAccessHandler);
	NetworkManager::registerHandler(MSG_REGISTER_OBJ, &RegisterObjectMsg::registerObjectHandler);
}

template<class Archive>
void HyflowMessage::registerMessageTypes(Archive & ar){
	ar.register_type(static_cast<ObjectAccessMsg*>(NULL));
	ar.register_type(static_cast<SynchronizeMsg*>(NULL));
	ar.register_type(static_cast<ObjectTrackerMsg*>(NULL));
	ar.register_type(static_cast<RegisterObjectMsg*>(NULL));
}

void HyflowMessage::test(){
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

	// create and open a character archive for output
	std::ofstream ofs("/tmp/HyflowMessage", std::ios::out);

	// create class instance
	vt_dstm::ObjectAccessMsg bq("3-0",true);
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
		vt_dstm::ObjectAccessMsg *bq1 =  (vt_dstm::ObjectAccessMsg *)r1.getMsg();
		if (bq1->getId().compare("3-0") == 0) {
			std::cout<< "HyflowMessage serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "HyflowMessage serialization Test FAILED!!!"<<std::endl;
		}
	}
}

}	/*namespace vt_dstm*/

