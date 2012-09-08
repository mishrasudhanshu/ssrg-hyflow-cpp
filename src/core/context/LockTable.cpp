/*
 * LockTable.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#include "LockTable.h"
#include "../../util/logging/Logger.h"

namespace vt_dstm {

int32_t LockTable::REMOTE = 1 << 31;
int32_t LockTable::LOCK = 1 << 30;
int32_t LockTable::UNLOCK = ~LOCK;

std::map<std::string, int32_t>	LockTable::lockmap;
boost::shared_mutex LockTable::rwMutex;

LockTable::LockTable() {}

LockTable::~LockTable() {}

//FIXME: Update once concurrentHashMap is fixed
bool LockTable::tryLock(std::string & objId) {
	boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
	std::map<std::string, int32_t>::iterator i = lockmap.find(objId);
	if ( i == lockmap.end()) {
		lockmap [objId] = 0 | LOCK;
		Logger::debug("LockTable : new lock created for %s\n", objId.c_str());
		return true;
	}
	int32_t versionLock = i->second;
	// Object is registered in lock table
	if (versionLock & LOCK) {
		Logger::debug("LockTable :  %s already locked\n", objId.c_str());
		return false;
	}
	lockmap [objId] = versionLock | LOCK;
	Logger::debug("LockTable :  %s locked successfully\n", objId.c_str());
	return true;
}

bool LockTable::isLocked(std::string & objId) {
	boost::shared_lock<boost::shared_mutex> readLock(rwMutex);
	std::map<std::string, int32_t>::iterator i = lockmap.find(objId);
	if ( i == lockmap.end()) {
		return false;
	}
	int32_t versionLock = i->second;
	// Object is registered in lock table
	if (versionLock & LOCK)
		return true;
	return false;
}

void LockTable::tryUnlock(std::string & objId) {
	boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
	std::map<std::string, int32_t>::iterator i = lockmap.find(objId);
	if ( i == lockmap.end()) {
		Logger::fatal("LockTable : on unlock call for %s not found\n", objId.c_str());
		return;
	}
	int32_t versionLock = i->second;
	// Object is registered in lock table
	if (versionLock & LOCK) {
		lockmap [objId] = versionLock & UNLOCK;
		Logger::debug("LockTable : Unlock successful for %s\n", objId.c_str());
		return;
	}
	Logger::fatal("LockTable : Request object %s was already unlocked\n", objId.c_str());
}

} /* namespace vt_dstm */
