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

tbb::concurrent_hash_map<std::string, int32_t>	LockTable::lockmap;

LockTable::LockTable() {}

LockTable::~LockTable() {}

//FIXME: Update once concurrentHashMap is fixed
bool LockTable::tryLock(std::string & objId, int32_t obVer) {
	tbb::concurrent_hash_map<std::string, int32_t>::accessor a;
	if (lockmap.insert(a, objId)) {		// Object does not exist in lock table
		a->second = obVer | LOCK;
		LOG_DEBUG("LockTable : new lock created for %s version %d\n", objId.c_str(), obVer);
		return true;
	} else {						// Object exit in lock Table
		int32_t versionLock = a->second;
		// Object is registered in lock table
		if (versionLock & LOCK) {	// Object is locked
			if ( (versionLock & UNLOCK) >= obVer) {	// Some one already locked same or newer version of object
				LOG_DEBUG("LockTable :  %s already locked for version %d\n", objId.c_str(), obVer);
				return false;
			} else {		// Some one had locked older version of object
				a->second = obVer | LOCK;
				LOG_DEBUG("LockTable : older Lock exist - locked successfully version %d\n", obVer);
				return true;
			}
		} else {	// Object is unlock
			if ( versionLock > obVer) {	// User is requesting lock for older version of object
				LOG_DEBUG("LockTable : Older object lock requested %d < lock version %d\n", obVer, versionLock);
				return false;
			} else if ( versionLock == obVer ){
				// get version lock : not required here as already in map write lock, but will be required
				// for a concurrent hashMap
				if ( obVer < DirectoryManager::getObjectVersion(objId)) {
					LOG_DEBUG("LockTable : Object version increased since last read %d\n", obVer);
					return false;
					// Unlock now in concurrent implementation
				}
			}
			a->second = obVer | LOCK;
			LOG_DEBUG("LockTable :  %s locked successfully version %d\n", objId.c_str(), obVer);
			return true;
		}
	}
}


// TODO: Remove it, should not be required
bool LockTable::isLocked(std::string & objId, int32_t obVer) {
	tbb::concurrent_hash_map<std::string, int32_t>::const_accessor a;
	if (!lockmap.find(a, objId)) {
		return false;
	}
	int32_t versionLock = a->second;
	// Object is registered in lock table
	if (versionLock & LOCK) {
		if ((versionLock & UNLOCK) == obVer) {
			return true;
		}
	}
	return false;
}

void LockTable::tryUnlock(std::string & objId, int32_t obVer) {
	tbb::concurrent_hash_map<std::string, int32_t>::accessor a;
	if ( lockmap.insert(a, objId)) {
		Logger::fatal("LockTable : on unlock call for %s not found\n", objId.c_str());
		return;
	}
	int32_t versionLock = a->second;
	// Object is registered in lock table
	if (versionLock & LOCK) {
		if ( (versionLock & UNLOCK) > obVer) {
			LOG_DEBUG("LockTable : Can not unlock %s of version %d already overwritten\n",objId.c_str(), obVer);
			return;
		} else if ((versionLock & UNLOCK) < obVer){
			Logger::fatal("LockTable : Impossible request Unlock %s of version %d of future\n",objId.c_str(), obVer);
			return;
		}
		a->second = versionLock & UNLOCK;
		LOG_DEBUG("LockTable : Unlock successful for %s\n", objId.c_str());
		return;
	}
	LOG_DEBUG("LockTable : Request object %s was already unlocked\n", objId.c_str());
}

} /* namespace vt_dstm */
