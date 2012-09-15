/*
 * LocalCache.h
 *
 *  Created on: Sep 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOCALCACHE_H_
#define LOCALCACHE_H_

#include <string>
#include <map>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "../../HyflowObject.h"

namespace vt_dstm {

class LocalCache {
	std::map<std::string, HyflowObject*> map;
	boost::shared_mutex rwMutex;
public:
	LocalCache() {};
	virtual ~LocalCache() {};

	/*
	 * Creates new copy of object on heap and saves its pointer in cache
	 */
	void updateObject(HyflowObject *object) {
		std::string objId = object->getId();
		HyflowObject* objectCopy = NULL;
		object->getClone(&objectCopy);
		{
			boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
			std::map<std::string, HyflowObject*>::iterator i = map.find(objId);
			if ( i == map.end()) {
				map[objId] = objectCopy;
			} else {
				delete i->second;
				map.erase(i);
				map[objId] = objectCopy;
			}
		}
	}

	/*
	 * Create new copy of object on heap and return its pointer to calling function
	 */
	HyflowObject* getObject(std::string objId) {
		HyflowObject* objectCopy = NULL;
		HyflowObject* obj=NULL;
		{
			boost::shared_lock<boost::shared_mutex> readLock(rwMutex);
			std::map<std::string, HyflowObject*>::iterator i = map.find(objId);
			if ( i != map.end()) {
				obj = i->second;
				obj->getClone(&objectCopy);
			}
			return objectCopy;
		}
	}

	/*
	 * Provides the current object version
	 */
	int32_t  getObjectVersion(std::string & id) {
		boost::shared_lock<boost::shared_mutex> readLock(rwMutex);
		std::map<std::string, HyflowObject*>::iterator i = map.find(id);
		if ( i != map.end()) {
			return i->second->getVersion();
		}else {
			throw "Requested object does not exist in local cache";
			return 0;
		}
	}
};

} /* namespace vt_dstm */

#endif /* LOCALCACHE_H_ */
