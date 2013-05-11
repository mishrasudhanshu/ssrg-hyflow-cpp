/*
 * AbstractLockTable.cpp
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#include "AbstractLockTable.h"

#include "../../util/logging/Logger.h"

namespace vt_dstm {

tbb::concurrent_hash_map<std::string, AbstractLock*> AbstractLockTable::absLockmap;

AbstractLockTable::AbstractLockTable() {}

AbstractLockTable::~AbstractLockTable() {}

bool AbstractLockTable::tryLock(AbstractLock *ablock, bool isRead) {
	std::string lockName = ablock->getLockName();
	AbstractLock *lockCopy = NULL;
	ablock->getClone(&lockCopy);
	tbb::concurrent_hash_map<std::string, AbstractLock*>::accessor a;
	unsigned long long txnId = ablock->getRequesterTxnId();
	if (!absLockmap.insert(a, lockName)) {
		if ( isRead ) {
			if ( a->second->isReadLockable(txnId) ){
				a->second->readlock(txnId);
				return true;
			}
			return false;
		}else {
			ablock->getClone(&lockCopy);
			if ( a->second->isWriteLockable(txnId) ) {
				lockCopy->writelock(txnId);
				AbstractLock *oldLock = a->second;
				a->second = lockCopy;
				delete oldLock;
				LOG_DEBUG("ALT :Lock %s for Txn %llu successful\n", lockName.c_str(), ablock->getRequesterTxnId());
				return true;
			}
			return false;
		}
	}else {
		ablock->getClone(&lockCopy);
		if (isRead) {
			lockCopy->readlock();
		}else {
			lockCopy->writelock();
		}
		a->second = lockCopy;
		LOG_DEBUG("ALT :New Lock %s for Txn %llu created\n", lockName.c_str(), ablock->getRequesterTxnId());
		return true;
	}
}

void AbstractLockTable::unlock(AbstractLock *ablock, bool read) {
	std::string lockName = ablock->getLockName();
	unsigned long long txnId = ablock->getRequesterTxnId();
	tbb::concurrent_hash_map<std::string, AbstractLock*>::accessor a;
	if (absLockmap.find(a, lockName)) {
		a->second->unlock(read, txnId);
		LOG_DEBUG("ALT :Unlock %s unlocked successfully\n", lockName.c_str());
	}else {
		Logger::fatal("ALT :Unlock %s not found\n", lockName.c_str());
	}
}

} /* namespace vt_dstm */
