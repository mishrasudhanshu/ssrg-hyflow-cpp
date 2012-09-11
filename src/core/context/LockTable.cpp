/*
 * LockTable.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#include "LockTable.h"
#include "../../util/logging/Logger.h"
#include "../directory/DirectoryManager.h"

namespace vt_dstm {

int32_t LockTable::REMOTE = 1 << 31;
int32_t LockTable::LOCK = 1 << 30;
int32_t LockTable::UNLOCK = ~LOCK;

std::map<std::string, int32_t>	LockTable::lockmap;
boost::shared_mutex LockTable::rwMutex;

LockTable::LockTable() {}

LockTable::~LockTable() {}

//FIXME: Update once concurrentHashMap is fixed
bool LockTable::tryLock(std::string & objId, int32_t obVer) {
	boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
	std::map<std::string, int32_t>::iterator i = lockmap.find(objId);
	if ( i == lockmap.end()) {		// Object does not exist in lock table
		lockmap [objId] = obVer | LOCK;
		Logger::debug("LockTable : new lock created for %s version %d\n", objId.c_str(), obVer);
		return true;
	} else {						// Object exit in lock Table
		int32_t versionLock = i->second;
		// Object is registered in lock table
		if (versionLock & LOCK) {	// Object is locked
			if ( (versionLock & UNLOCK) >= obVer) {	// Some one already locked same or newer version of object
				Logger::debug("LockTable :  %s already locked for version %d\n", objId.c_str(), obVer);
				return false;
			} else {		// Some one had locked older version of object
				lockmap [objId] = obVer | LOCK;
				Logger::debug("LockTable : older Lock exist - locked successfully version %d\n", obVer);
				return true;
			}
		} else {	// Object is unlock
			if ( versionLock > obVer) {	// User is requesting lock for older version of object
				Logger::debug("LockTable : Older object lock requested %d < lock version %d\n", obVer, versionLock);
				return false;
			} else if ( versionLock == obVer ){
				// get version lock : not required here as already in map write lock, but will be required
				// for a concurrent hashMap
				if ( obVer < DirectoryManager::getObjectVersion(objId)) {
					Logger::debug("LockTable : Object version increased since last read %d\n", obVer);
					return false;
					// Unlock now in concurrent implementation
				}
			}
			lockmap [objId] = obVer | LOCK;
			Logger::debug("LockTable :  %s locked successfully version %d\n", objId.c_str(), obVer);
			return true;
		}
	}
}


// TODO: Remove it, should not be required
bool LockTable::isLocked(std::string & objId, int32_t obVer) {
	boost::shared_lock<boost::shared_mutex> readLock(rwMutex);
	std::map<std::string, int32_t>::iterator i = lockmap.find(objId);
	if ( i == lockmap.end()) {
		return false;
	}
	int32_t versionLock = i->second;
	// Object is registered in lock table
	if (versionLock & LOCK) {
		if ((versionLock & UNLOCK) == obVer) {
			return true;
		}
	}
	return false;
}

void LockTable::tryUnlock(std::string & objId, int32_t obVer) {
	boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
	std::map<std::string, int32_t>::iterator i = lockmap.find(objId);
	if ( i == lockmap.end()) {
		Logger::fatal("LockTable : on unlock call for %s not found\n", objId.c_str());
		return;
	}
	int32_t versionLock = i->second;
	// Object is registered in lock table
	if (versionLock & LOCK) {
		if ( (versionLock & UNLOCK) > obVer) {
			Logger::debug("LockTable : Can not unlock %s of version %d already overwritten\n",objId.c_str(), obVer);
			return;
		} else if ((versionLock & UNLOCK) < obVer){
			Logger::fatal("LockTable : Impossible request Unlock %s of version %d of future\n",objId.c_str(), obVer);
			return;
		}
		lockmap [objId] = versionLock & UNLOCK;
		Logger::debug("LockTable : Unlock successful for %s\n", objId.c_str());
		return;
	}
	Logger::fatal("LockTable : Request object %s was already unlocked\n", objId.c_str());
}

} /* namespace vt_dstm */
