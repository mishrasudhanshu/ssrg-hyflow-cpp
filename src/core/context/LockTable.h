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

class LockTable {
	static int32_t REMOTE;
	static int32_t LOCK;
	static int32_t UNLOCK;

	static tbb::concurrent_hash_map<std::string, int32_t> lockmap;
public:
	LockTable();
	virtual ~LockTable();

	static bool isLocked(std::string & objId, int32_t obVer);
	static bool tryLock(std::string & objId, int32_t obVer);
	static void tryUnlock(std::string & objId,  int32_t obVer);
};

} /* namespace vt_dstm */

#endif /* LOCKTABLE_H_ */
