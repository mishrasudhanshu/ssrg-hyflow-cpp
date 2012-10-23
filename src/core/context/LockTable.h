/*
 * LockTable.h
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOCKTABLE_H_
#define LOCKTABLE_H_

#include "tbb/concurrent_hash_map.h"

namespace vt_dstm {

class LockValue {
	int32_t lockVersion;
	unsigned long long ownerTxnId;
public:
	LockValue() {
		lockVersion = 0;
		ownerTxnId = 0;
	}

	LockValue(int32_t ver, unsigned long long ownerTxn) {
		lockVersion = ver;
		ownerTxnId = ownerTxn;
	}

	int32_t getLockVersion() const {
		return lockVersion;
	}

	void setLockVersion(int32_t lockVersion) {
		this->lockVersion = lockVersion;
	}

	unsigned long long getOwnerTxnId() const {
		return ownerTxnId;
	}

	void setOwnerTxnId(unsigned long long ownerTxnId) {
		this->ownerTxnId = ownerTxnId;
	}
};

class LockTable {
	static int32_t REMOTE;
	static int32_t LOCK;
	static int32_t UNLOCK;

	static tbb::concurrent_hash_map<std::string, LockValue> lockmap;
public:
	LockTable();
	virtual ~LockTable();

	static bool isLocked(std::string & objId, int32_t obVer, unsigned long long txnId);
	static bool tryLock(std::string & objId, int32_t obVer, unsigned long long txnId);
	static void tryUnlock(std::string & objId,  int32_t obVer, unsigned long long txnId);
};

} /* namespace vt_dstm */

#endif /* LOCKTABLE_H_ */
