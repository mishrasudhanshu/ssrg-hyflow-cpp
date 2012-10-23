/*
 * ReadValidationMsg.h
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef READVALIDATIONMSG_H_
#define READVALIDATIONMSG_H_

#include <string>
#include "../BaseMessage.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

/**
 * TODO: Multiple request can be merged as object array
 * version array send to same node
 */
class ReadValidationMsg: public vt_dstm::BaseMessage {
	std::string objectId;
	int32_t objectVersion;
	bool validationResponse;
	bool request;
	unsigned long long txnId;

	friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	ReadValidationMsg();
	ReadValidationMsg(std::string objectId, int32_t obVer, bool isRequest, unsigned long long tid);
	virtual ~ReadValidationMsg();
	std::string getObjectId() const;
	void setObjectId(std::string objectId);

	static void readValidationHandle(HyflowMessage & msg);
	bool isRequest() const;
	void setRequest(bool request);

	void serializationTest();
};

} /* namespace vt_dstm */

#endif /* READVALIDATIONMSG_H_ */
