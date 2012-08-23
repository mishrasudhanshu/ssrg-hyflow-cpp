/*
 * list.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "list.h"

namespace vt_dstm {

template<class Archive>
void list::serialize(Archive & ar, const unsigned int version) {
	// save/load base class information
	ar & boost::serialization::base_object<HyflowObject>(*this);
	ar & street1 & street2;
}

list::list(const std::string & Id, const int & v, const std::string &s1, const std::string & s2){
	hyId = Id;
	hyVersion = v;
	street1 = s1;
	street2 = s2;
}

void list::print(){
	std::cout<<"street 1:"<<street1<<" street2:"<<street2<<std::endl;
}

} /* namespace testCpp */

// Serialisation Test of object
int lmain() {
	// create and open a character archive for output
	std::string id = "0-0";
	std::ofstream ofs("filename", std::ios::out);

	// create class instance
	vt_dstm::list b(id, 1, id, id);

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << b;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::list b1;
	{
		// create and open an archive for input
		std::ifstream ifs("filename", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> b1;
		// archive and stream closed when destructors are called
		b1.print();
	}
	return 0;
}
