/*
 * LockTable.h
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOCKTABLE_H_
#define LOCKTABLE_H_

#include <boost/thread/shared_mutex.hpp>
#include "../../util/concurrent/ConcurrentHashMap.h"

namespace vt_dstm {

class LockTable {
	static int32_t REMOTE;
	static int32_t LOCK;
	static int32_t UNLOCK;

//	static ConcurrentHashMap<std::string, int32_t>	lockmap;
	static std::map<std::string, int32_t> lockmap;
	static boost::shared_mutex rwMutex;
public:
	LockTable();
	virtual ~LockTable();

	static bool isLocked(std::string & objId);
	static bool tryLock(std::string & objId);
	static void tryUnlock(std::string & objId);
};

} /* namespace vt_dstm */

#endif /* LOCKTABLE_H_ */
