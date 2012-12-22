/*
 * AbstractLock.h
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTLOCK_H_
#define ABSTRACTLOCK_H_

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

namespace vt_dstm {

class AbstractLock {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

	std::string highestObjectName;
	std::string lockName;
	// Mutual exclusion lock for current
	bool absLock;
	unsigned long long txnId;
public:
	AbstractLock();
	AbstractLock(std::string highestObjectName, std::string lockName, unsigned long long txnId);
	virtual ~AbstractLock();

	bool isLocked();
	bool lock(bool isRead);
	void unlock();
	int getTracker();

	void setAbsLock(bool absLock) {
		this->absLock = absLock;
	}

	std::string getLockName() const {
		return lockName;
	}

	void getClone(AbstractLock **abl) {
		AbstractLock* absLockCopy = new AbstractLock();
		absLockCopy->highestObjectName = highestObjectName;
		absLockCopy->lockName = lockName;
		absLockCopy->absLock = absLock;
		absLockCopy->txnId = txnId;
		*abl = absLockCopy ;
	}

	static void serializationTest();

	unsigned long long getTxnId() const {
		return txnId;
	}
};

} /* namespace vt_dstm */

#endif /* ABSTRACTLOCK_H_ */
