/*
 * RegisterObjectMsg.h
 *
 *  Created on: Aug 28, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef REGISTEROBJECTMSG_H_
#define REGISTEROBJECTMSG_H_

#include <string>
#include "BaseMessage.h"

namespace vt_dstm {

class RegisterObjectMsg: public vt_dstm::BaseMessage {
	HyflowObject *object;
	unsigned long long txnId;
	std::string objectId;

	friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	RegisterObjectMsg(HyflowObject *obj, unsigned long long tid);
	RegisterObjectMsg(std::string id, unsigned long long tid);
	virtual ~RegisterObjectMsg();

	static void registerObjectHandler(HyflowMessage & msg);
	HyflowObject *getObject();
	void serializationTest();
};

} /* namespace vt_dstm */

#endif /* REGISTEROBJECTMSG_H_ */
