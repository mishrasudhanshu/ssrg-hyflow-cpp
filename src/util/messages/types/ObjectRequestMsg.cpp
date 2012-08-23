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
#include "ObjectRequestMsg.h"

namespace vt_dstm {

ObjectRequestMsg::ObjectRequestMsg(std::string Id, bool rw) {
	id = Id;
	isRead = rw;
}

ObjectRequestMsg::~ObjectRequestMsg() {}

template<class Archive>
void ObjectRequestMsg::serialize(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<HyflowMessage>(*this);
	ar & id;
	ar & isRead;
}

void ObjectRequestMsg::print(){
	std::cout <<" Id "<<id<<" isRead "<<isRead;
}

} /* namespace vt_dstm */

// Serialisation Test of object
int serializationTest_ObjReq() {
	// create and open a character archive for output
	std::ofstream ofs("filename", std::ios::out);

	// create class instance
	vt_dstm::ObjectRequestMsg  res("0-0", true);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its original state
	vt_dstm::ObjectRequestMsg r1;
	{
		// create and open an archive for input
		std::ifstream ifs("filename", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		r1.print();
	}
	return 0;
}
