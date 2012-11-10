/*
 * DummyTestMsg.h
 *
 *  Created on: Nov 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DUMMYTESTMSG_H_
#define DUMMYTESTMSG_H_

#include <string>
#include "../BaseMessage.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

class DummyTestMsg: public vt_dstm::BaseMessage  {
	std::string msg;
	bool request;
	friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	DummyTestMsg();
	DummyTestMsg(std::string msg);
	virtual ~DummyTestMsg();
	static void DummyTestMsgHandler(HyflowMessage& m);
	void serializationTest();
	std::string getMsg() const;
	void setMsg(std::string msg);
	bool isRequest() const;
	void setRequest(bool request);
};

} /* namespace vt_dstm */

#endif /* DUMMYTESTMSG_H_ */
