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
#include "../HyflowDirectory.h"
#include "../../HyflowObject.h"
#include "../../../util/concurrent/ConcurrentHashMap.h"

namespace vt_dstm {

class TrackerDirectory: public vt_dstm::HyflowDirectory {
	static ConcurrentHashMap<std::string, int> directory;
	static ConcurrentHashMap<std::string, HyflowObject*> local;
	void updateObjectToLocal(HyflowObject &obj);
	void updateObjectToDirectory(HyflowObject &obj);
	int getTracker(std::string objectId);
	void registerObjectLocally(std::string objId, int owner, unsigned long long txn);
	void unregisterObjectLocally(std::string id, unsigned long long txn);
public:
	TrackerDirectory();
	virtual ~TrackerDirectory();

	HyflowObject* locate(std::string id, bool rw, unsigned long long txn);

	void locateAsync(std::string id, bool rw, unsigned long long txn, HyflowObjectFuture & fu);

	void registerObject(HyflowObject & object, unsigned long long txn);

	void unregisterObject(HyflowObject & object, unsigned long long txn);

	HyflowObject & getObjectLocally(std::string id, bool rw);
	void updateObjectLocally(HyflowObject & obj);

	int getObjectLocation(std::string objId);
};

} /* namespace vt_dstm */

#endif /* TRACKERDIRECTORY_H_ */
