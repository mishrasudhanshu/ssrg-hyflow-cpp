/*
 * AbstractLock.h
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTLOCK_H_
#define ABSTRACTLOCK_H_

#include <string>

namespace vt_dstm {

class AbstractLock {
	std::string highestObjectName;
	std::string lockName;
	// Mutual exclusion lock
	bool absLock;
	unsigned long long txnId;
public:
	AbstractLock();
	virtual ~AbstractLock();

	bool isLocked();
	bool lock(bool isRead);
	void unlock();

	void getTracker();

	std::string getHighestObjectName() {
		return highestObjectName;
	}

	void setHighestObjectName(std::string highestObjectName) {
		this->highestObjectName = highestObjectName;
	}

	bool isLock() const {
		return absLock;
	}

	void setLock(bool alock) {
		this->absLock = alock;
	}

	std::string getLockName() const {
		return lockName;
	}

	void setLockName(std::string lockName) {
		this->lockName = lockName;
	}

	unsigned long long getTxnId() const {
		return txnId;
	}

	void setTxnId(unsigned long long txnId) {
		this->txnId = txnId;
	}
};

} /* namespace vt_dstm */

#endif /* ABSTRACTLOCK_H_ */
