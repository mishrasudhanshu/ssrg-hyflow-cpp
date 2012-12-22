/*
 * AbstractLock.cpp
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "AbstractLock.h"
#include "../../util/networking/NetworkManager.h"


namespace vt_dstm {

AbstractLock::AbstractLock() {
	absLock = false;
	txnId = 0;
}

AbstractLock::AbstractLock(std::string hO, std::string lN, unsigned long long tid) {
	highestObjectName = hO;
	lockName = lN;
	txnId = tid;
	absLock = false;
}

AbstractLock::~AbstractLock() {}

template<class Archive>
void AbstractLock::serialize(Archive & ar, const unsigned int version) {
	ar & highestObjectName;
	ar & lockName;
	ar & absLock;
	ar & txnId;
}

bool AbstractLock::isLocked() {
	return absLock;
}

bool AbstractLock::lock(bool isRead) {
	if (!absLock) {
		absLock = true;
		return true;
	}else {
		return false;
	}
}

void AbstractLock::unlock() {
	absLock = false;
}

int AbstractLock::getTracker() {
	int val =  atoi(lockName.substr(0, lockName.find('-')).c_str());
	return val%NetworkManager::getNodeCount();
}

// Serialisation Test of object
void AbstractLock::serializationTest() {
	{
		// create and open a character archive for output
		std::ofstream ofs("/tmp/AbstractLock", std::ios::out);

		// create class instance
		AbstractLock absLock("Test", "0", 0);

		// save data to archive
		{
			boost::archive::text_oarchive oa(ofs);
			// write class instance to archive
			oa << absLock;
			// archive and stream closed when destructors are called
		}

		// ... some time later restore the class instance to its original state
		AbstractLock r1;
		{
			// create and open an archive for input
			std::ifstream ifs("/tmp/AbstractLock", std::ios::in);
			boost::archive::text_iarchive ia(ifs);
			// read class state from archive
			ia >> r1;
			// archive and stream closed when destructors are called
			if (r1.getLockName().compare("Test-0") == 0) {
				std::cout<< "AbstractLock serialization Test passed"<<std::endl;
			}else {
				std::cerr<< "AbstractLock serialization Test FAILED!!!"<<std::endl;
			}
		}
	}
}

} /* namespace vt_dstm */
