/*
 * LocalCacheIntel.h
 *
 *  Created on: Sep 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOCALCACHETBB_H_
#define LOCALCACHETBB_H_

#include <string>
#include <map>
#include "tbb/concurrent_hash_map.h"
#include "../../HyflowObject.h"

namespace vt_dstm {

class LocalCacheTbb {
	tbb::concurrent_hash_map<std::string, HyflowObject*>* map;
public:
	LocalCacheTbb(int cacheSize) {
		map = new tbb::concurrent_hash_map<std::string, HyflowObject*>(cacheSize);
	}

	virtual ~LocalCacheTbb() {
		delete map;
	}

	/*
	 * Creates new copy of object on heap and saves its pointer in cache
	 */
	void updateObject(HyflowObject *object) {
		const std::string & objId = object->getId();
		HyflowObject* objectCopy = NULL;
		object->getClone(&objectCopy);
		{
			tbb::concurrent_hash_map<std::string, HyflowObject*>::accessor a;
			if (!map->insert(a,objId)) {
				HyflowObject *saveObject = a->second;
				a->second = NULL;
				delete saveObject;
			}
			a->second = objectCopy;
		}
	}

	/*
	 * Create new copy of object on heap and return its pointer to calling function
	 */
	HyflowObject* getObject(const std::string & objId) {
		HyflowObject* objectCopy = NULL;
		HyflowObject* obj=NULL;
		{
			tbb::concurrent_hash_map<std::string, HyflowObject*>::const_accessor a;
			if (map->find(a,objId)) {
				obj = a->second;
				if(!obj) {
					throw "NULL Object Found!!";
				}
				obj->getClone(&objectCopy);
				return objectCopy;
			} else {
				throw "No Object Found!!";
			}
		}
		return objectCopy;
	}

	/*
	 * Provides the current object version
	 */
	int32_t  getObjectVersion(const std::string & objId) {
		HyflowObject* obj=NULL;
		tbb::concurrent_hash_map<std::string, HyflowObject*>::const_accessor a;
		if (map->find(a,objId)) {
			obj = a->second;
			if(!obj) {
				throw "NULL Object Found!!";
			}
			return obj->getVersion();
		} else {
			throw "No Object Found!!";
		}
		return -1;

	}
};

} /* namespace vt_dstm */

#endif /* LOCALCACHETBB_H_ */
