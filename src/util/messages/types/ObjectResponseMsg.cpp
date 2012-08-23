/*
 * ObjectResponseMsg.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include "ObjectResponseMsg.h"
#include "../../../benchMarks/tm/bank/BankAccount.h"

namespace vt_dstm {

ObjectResponseMsg::ObjectResponseMsg(){}

template<class Archive>
void ObjectResponseMsg::serialize(Archive & ar, const unsigned int version){
	ar & boost::serialization::base_object<HyflowMessage>(*this);
	ar & object;
}

}

// Serialisation Test of object
int serializationTest_ObjRes() {
	// create and open a character archive for output
	std::ofstream ofs("filename", std::ios::out);

	vt_dstm::BankAccount bac(1000,"0-0");
	// create class instance
	vt_dstm::ObjectResponseMsg<vt_dstm::BankAccount>  res;
	res.setObject(bac);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << res;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::ObjectResponseMsg<vt_dstm::BankAccount>  r1;
	vt_dstm::BankAccount b1;
	{
		// create and open an archive for input
		std::ifstream ifs("filename", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> r1;
		// archive and stream closed when destructors are called
		b1 = r1.getObject();
		b1.print();
	}
	return 0;
}


