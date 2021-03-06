/*
 * TrackerDirectory.h
 *
 *  Created on: Aug 28, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TRACKERDIRECTORY_H_
#define TRACKERDIRECTORY_H_

#include <string>
#include <map>
#include "tbb/concurrent_hash_map.h"
#include "../HyflowDirectory.h"
#include "../../HyflowObject.h"
#include "LocalCacheTbb.h"
#include "../../../util/concurrent/ConcurrentHashMap.h"

namespace vt_dstm {

class TrackerDirectory: public vt_dstm::HyflowDirectory {
	tbb::concurrent_hash_map<std::string, int>* directory;
	LocalCacheTbb* local;
	void updateObjectToLocal(HyflowObject* obj);
	void updateObjectToDirectory(HyflowObject* obj);
	int getTracker(std::string & objectId);
	void registerObjectLocally(std::string & objId, int owner, unsigned long long txn);
	void unregisterObjectLocally(std::string & id, unsigned long long txn);
public:
	TrackerDirectory();
	virtual ~TrackerDirectory();

	HyflowObject* locate(std::string & id, bool rw, unsigned long long txn);

	void locateAsync(std::string & id, bool rw, unsigned long long txn, HyflowObjectFuture & fu);

	void registerObject(HyflowObject* object, unsigned long long txn);
	void registerObjectWait(HyflowObject* object, unsigned long long txn);

	void unregisterObject(HyflowObject* object, unsigned long long txn);

	HyflowObject* getObjectLocally(std::string & id, bool rw);
	void updateObjectLocally(HyflowObject* obj);

	int getObjectLocation(std::string & objId);

	int32_t getObjectVersion(std::string & objId);
};

} /* namespace vt_dstm */

#endif /* TRACKERDIRECTORY_H_ */
