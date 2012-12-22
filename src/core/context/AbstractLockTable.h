/*
 * AbstractLockTable.h
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTLOCKTABLE_H_
#define ABSTRACTLOCKTABLE_H_

#include <string>
#include <tbb/concurrent_hash_map.h>

#include "AbstractLock.h"

namespace vt_dstm {

class AbstractLockTable {
	/*
	 * Contains the lock name and lock table
	 * lock name : is unique name to access abstract lock
	 * AbstrackLock : is created to support higher level locking in open nesting
	 */
	static tbb::concurrent_hash_map<std::string, AbstractLock*> absLockmap;
public:
	AbstractLockTable();
	virtual ~AbstractLockTable();

	/*
	 * If lock is successful, the Abstract lock values will be used
	 */
	static bool isLocked(std::string lockName);
	static bool tryLock(std::string lockName, AbstractLock *lock, bool isRead);
	static void unlock(std::string lockName);
};

} /* namespace vt_dstm */

#endif /* ABSTRACTLOCKTABLE_H_ */
