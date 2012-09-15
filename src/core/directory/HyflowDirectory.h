/*
 * HyflowDirectory.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWDIRECTORY_H_
#define HYFLOWDIRECTORY_H_

#include <string>
#include "../HyflowObject.h"
#include "../HyflowObjectFuture.h"

namespace vt_dstm {

class HyflowDirectory {
public:
	HyflowDirectory(){};
	virtual ~HyflowDirectory(){};

	virtual void registerObjectLocally(std::string & objId, int owner, unsigned long long txn)=0;
	virtual void unregisterObjectLocally(std::string & objId, unsigned long long txn)=0;
	/**
	 * This function call blocks until object is not returned back
	 */
	virtual HyflowObject* locate(std::string & id, bool rw, unsigned long long txn)=0;
	/**
	 * This function call return immediately user can check availability of
	 * object later.
	 */
	virtual void locateAsync(std::string & id, bool rw, unsigned long long txn, HyflowObjectFuture & fu)=0;
	/**
	 * Register object in the cluster
	 */
	virtual void registerObject(HyflowObject* object, unsigned long long txn)=0;
	/**
	 * Register object with wait in the cluster
	 */
	virtual void registerObjectWait(HyflowObject* object, unsigned long long txn)=0;
	/**
	 * Unregister object from the cluster
	 */
	virtual void unregisterObject(HyflowObject* object, unsigned long long txn)=0;
	/**
	 * Function used by message handler to access the object
	 */
	virtual HyflowObject* getObjectLocally(std::string & id, bool rw)=0;
	/**
	 * Update local object
	 */
	virtual void updateObjectLocally(HyflowObject* obj)=0;
	/**
	 * Track the object location
	 */
	virtual int getObjectLocation(std::string & objId)=0;
	/*
	 * Get object version
	 */
	virtual int32_t getObjectVersion(std::string & objId) = 0;
};

} /* namespace vt_dstm */

#endif /* HYFLOWDIRECTORY_H_ */
