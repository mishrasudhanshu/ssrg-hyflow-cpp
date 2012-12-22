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

bool AbstractLockTable::isLocked(std::string lockName) {
	tbb::concurrent_hash_map<std::string, AbstractLock*>::const_accessor a;
	if (absLockmap.find(a, lockName)) {
		return a->second->isLocked();
	}else {
		return true;
	}
}

bool AbstractLockTable::tryLock(std::string lockName, AbstractLock *ablock, bool isRead) {
	AbstractLock *lockCopy = NULL;
	ablock->getClone(&lockCopy);
	tbb::concurrent_hash_map<std::string, AbstractLock*>::accessor a;
	if (!absLockmap.insert(a, lockName)) {
		if (!a->second->isLocked()) {
			lockCopy->lock(isRead);
			AbstractLock *oldLock = a->second;
			a->second = lockCopy;
			delete oldLock;
			LOG_DEBUG("ALT :Lock %s for Txn %llu successful\n", lockName.c_str(), ablock->getTxnId());
			return true;
		}else {
			// Check if owner transaction is same
			if (a->second->getTxnId() == ablock->getTxnId()) {
				LOG_DEBUG("ALT :Recursive Lock %s for Txn %llu\n", lockName.c_str(), ablock->getTxnId());
				return true;
			}
			return false;
		}
	}else {
		lockCopy->lock(isRead);
		a->second = lockCopy;
		LOG_DEBUG("ALT :New Lock %s for Txn %llu created\n", lockName.c_str(), ablock->getTxnId());
		return true;
	}
}

void AbstractLockTable::unlock(std::string lockName) {
	tbb::concurrent_hash_map<std::string, AbstractLock*>::accessor a;
	if (absLockmap.find(a, lockName)) {
		if (!a->second->isLocked()) {
			Logger::fatal("ALT :Unlock %s lock already unlocked\n", lockName.c_str());
		}else {
			a->second->unlock();
			LOG_DEBUG("ALT :Unlock %s unlocked successfully\n", lockName.c_str());
		}
	}else {
		Logger::fatal("ALT :Unlock %s not found\n", lockName.c_str());
	}
}

} /* namespace vt_dstm */
