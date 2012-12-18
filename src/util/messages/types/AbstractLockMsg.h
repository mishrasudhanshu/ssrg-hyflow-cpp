/*
 * AbstractLockMsg.h
 *
 *  Created on: Dec 15, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTLOCKMSG_H_
#define ABSTRACTLOCKMSG_H_

#include "../BaseMessage.h"
#include "../HyflowMessage.h"
#include "../../../core/context/AbstractLock.h"

namespace vt_dstm {

class AbstractLockMsg: public vt_dstm::BaseMessage {
	unsigned long long txnId;
	bool doLock;
	bool request;
	bool read;
	AbstractLock* abstractLock;
	bool response;

	friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	AbstractLockMsg(AbstractLock* absLock, unsigned long long txnId, bool lock);
	virtual ~AbstractLockMsg();

	static void absLockAccessHandler(HyflowMessage& m);
};

} /* namespace vt_dstm */

#endif /* ABSTRACTLOCKMSG_H_ */
