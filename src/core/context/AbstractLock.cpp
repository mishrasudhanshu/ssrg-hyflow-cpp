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
#include "../../util/logging/Logger.h"

namespace vt_dstm {

AbstractLock::AbstractLock() {
	absLock = 0;
}

AbstractLock::AbstractLock(std::string hO, std::string lN, unsigned long long tid) {
	highestObjectName = hO;
	lockName = lN;
	requesterTxnId = tid;
	absLock = 0;
	fetched = false;
}

AbstractLock::~AbstractLock() {}

template<class Archive>
void AbstractLock::serialize(Archive & ar, const unsigned int version) {
	ar & highestObjectName;
	ar & lockName;
	ar & absLock;
	ar & requesterTxnId;
	ar & txnIds;
	ar & fetched;
}

bool AbstractLock::isReadLockable(unsigned long long txnId) {
	if ( absLock > -1) {
		return true;
	}else {
		if ( txnId == requesterTxnId ) {
			LOG_DEBUG("ABSLCK : Currently write locked by requesting txn %llu\n", txnId);
			return true;
		}
		return false;
	}
}

bool AbstractLock::isWriteLockable(unsigned long long txnId) {
	if ( absLock == -1) { // Write locked
		if ( txnId == requesterTxnId ) {
			LOG_DEBUG("ABSLCK : Currently write locked by requesting txn %llu\n", txnId);
			return true;
		}
		return false;
	}else if ( absLock == 0) { // Not locked
		return true;
	}else if ( absLock == 1) { // Read locked by 1
		if ( txnIds.find(txnId) != txnIds.end() ) {
			LOG_DEBUG("ABSLCK : Currently read locked by requesting txn %llu\n", txnId);
			return true;
		}
		return false;
	}else {	// Read locked by more than 1
		return false;
	}
}

void AbstractLock::readlock(unsigned long long txnId) {
	if ( (txnId == requesterTxnId) &&
			( absLock == -1 ) ){ // If write lock by me do nothing
		return;
	}
	txnIds.insert(txnId);
	absLock = txnIds.size();
}

void AbstractLock::readlock() {
	txnIds.insert(requesterTxnId);
	absLock = txnIds.size();
}

void AbstractLock::writelock(unsigned long long txnId) {
	requesterTxnId = txnId;
	absLock = -1;
	txnIds.clear();
}

void AbstractLock::writelock() {
	absLock = -1;
	txnIds.clear();
}

void AbstractLock::unlock(bool isRead, unsigned long long txnId) {
	if ( isRead ) {
		if ( absLock == -1) {
			if (requesterTxnId == txnId) {
				// Possible if object first took read lock later applied for write lock, just ignore
				LOG_DEBUG("ABSLCK : Lock %s is actually write locked by %llu\n", lockName.c_str(), requesterTxnId);
			}
			Logger::fatal("ABSLCK : Lock %s is locked by different owner %llu\n", lockName.c_str(), requesterTxnId);
		}else if ( absLock == 0 ) {
			Logger::fatal("ABSLCK : Lock %s is already unlocked owner %llu\n", lockName.c_str(), requesterTxnId);
		}else {
			txnIds.erase(txnId);
			absLock = txnIds.size();
		}
	} else {
		if ( absLock != -1 ) {
			Logger::fatal("ABSLCK : Lock %s is not write locked %d owner %llu\n", lockName.c_str(), absLock, requesterTxnId);
		}else {
			absLock = 0;
			requesterTxnId = 0;
			txnIds.clear();
		}
	}
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
