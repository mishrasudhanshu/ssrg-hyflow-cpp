/*
 * AbstractLock.h
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTLOCK_H_
#define ABSTRACTLOCK_H_

#include <string>
#include <set>
#include <boost/serialization/set.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

namespace vt_dstm {

class AbstractLock {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

	std::string highestObjectName;
	std::string lockName;
	// Shared lock for current
	int absLock;
	bool fetched;
	unsigned long long requesterTxnId;
	std::set<unsigned long long > txnIds;

public:
	AbstractLock();
	AbstractLock(std::string highestObjectName, std::string lockName, unsigned long long txnId);
	virtual ~AbstractLock();
	bool isWriteLockable(unsigned long long txnId);
	bool isReadLockable(unsigned long long txnId);
	void readlock();
	void readlock(unsigned long long txnId);
	void writelock();
	void writelock(unsigned long long txnId);
	void unlock(bool isRead, unsigned long long txnId);
	int getTracker();

	std::string getLockName() const
	{
		return lockName;
	}

	void getClone(AbstractLock** abl)
	{
		AbstractLock* absLockCopy = new AbstractLock();
		absLockCopy->highestObjectName = highestObjectName;
		absLockCopy->lockName = lockName;
		absLockCopy->absLock = absLock;
		absLockCopy->txnIds = txnIds;
		absLockCopy->requesterTxnId = requesterTxnId;
		*abl = absLockCopy;
	}

	static void serializationTest();

	bool isFetchted() const
	{
		return fetched;
	}

	void setFetchted(bool fetchted)
	{
		this->fetched = fetchted;
	}

	unsigned long long getRequesterTxnId() const
	{
		return requesterTxnId;
	}
};

} /* namespace vt_dstm */

#endif /* ABSTRACTLOCK_H_ */
