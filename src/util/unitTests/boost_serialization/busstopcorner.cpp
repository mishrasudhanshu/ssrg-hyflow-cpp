/*
 * busstopcorner.cpp
 *
 *  Created on: Aug 21, 2012
 *      Author: mishras[at]vt.edu
 */

#include "busstopcorner.h"

namespace vt_dstm{

bus_stop_corner::bus_stop_corner(const int & _lat, const int & _long,
    const std::string & _s1, const std::string & _s2
){
    	latitude = _lat;
    	longitude = _long;
    	street1 = _s1;
    	street2 = _s2;
}

void bus_stop_corner::print(){
	std::cout<<"street 1:"<<street1<<" street2:"<<street2<<std::endl;
}

// Serialisation Test of object
void bus_stop_corner::test() {
	// create and open a character archive for output
	std::ofstream ofs("filename", std::ios::out);

	// create class instance
	vt_dstm::bus_stop_corner bc(1,1,"s1","s2");

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << bc;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::bus_stop_corner b1;
	{
		// create and open an archive for input
		std::ifstream ifs("filename", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> b1;
		// archive and stream closed when destructors are called
		b1.print();
	}
}

}

