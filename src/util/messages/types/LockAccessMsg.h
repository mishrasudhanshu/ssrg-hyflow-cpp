/*
 * LockAccessMsg.h
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOCKACCESSMSG_H_
#define LOCKACCESSMSG_H_

#include <string>
#include "../BaseMessage.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

class LockAccessMsg: public vt_dstm::BaseMessage {
	/*
	 * Is it a lock request or unlock request
	 */
	bool lock;
	/*
	 * Is lock granted or not
	 */
	bool locked;
	/*
	 * Is lock-unlock request message or response message
	 */
	bool request;
	/*
	 * id of object for which lock-unlock is requested
	 */
	std::string objectId;
	/*
	 * Object version
	 */
	int32_t objVersion;

	friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	LockAccessMsg() {
		lock = false;
		locked = false;
		request = false;
		objectId = "x-x";
		objVersion = -1;
	};

	LockAccessMsg(std::string objId, int32_t obVer);
	virtual ~LockAccessMsg();

	std::string getObjectId() const;
	void setObjectId(std::string objectId);

	static void lockAccessHandler(HyflowMessage& m);
	void serializationTest();
	bool isLock() const;
	void setLock(bool lock);
	bool isLocked() const;
	void setLocked(bool locked);
	bool isRequest() const;
	void setRequest(bool request);
}
;

} /* namespace vt_dstm */

#endif /* LOCKACCESSMESSAGE_H_ */
