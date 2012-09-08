/*
 * DirectoryManager.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DIRECTORYMANAGER_H_
#define DIRECTORYMANAGER_H_

#include <string>

#include "../HyflowObject.h"
#include "HyflowDirectory.h"

namespace vt_dstm {

class DirectoryManager {
	static HyflowDirectory *directory;
public:
	static void DirectoryManagerInit();
	/**
	 * This function call blocks until object is not returned back
	 */
	static HyflowObject* locate(std::string id, bool rw, unsigned long long txn);
	/**
	 * This function call return immediately user can check availability of
	 * object later.
	 */
	static void locateAsync(std::string id, bool rw, unsigned long long txn, HyflowObjectFuture & fu);
	/**
	 * Register object in the cluster
	 */
	static void registerObject(HyflowObject & object, unsigned long long txn);
	static void registerObjectWait(HyflowObject & object, unsigned long long txn);
	static void registerObjectLocally(std::string objectId, int owner, unsigned long long txn);
	/**
	 * Unregister object from the cluster
	 */
	static void unregisterObject(HyflowObject & object, unsigned long long txn);
	static void unregisterObjectLocally(std::string objId, unsigned long long txn);
	/**
	 * Function used by message handler to access the object
	 */
	static HyflowObject* getObjectLocally(std::string id, bool rw);
	/**
	 * Update local object
	 */
	static void updateObjectLocally(HyflowObject & obj);
	/**
	 * Track the object location
	 */
	static int getObjectLocation(std::string objId);
};

} /* namespace vt_dstm */

#endif /* DIRECTORYMANAGER_H_ */
