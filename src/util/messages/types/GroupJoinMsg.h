/*
 * GroupJoinMsg.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef GROUPJOINMSG_H_
#define GROUPJOINMSG_H_

#include "../BaseMessage.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

class GroupJoinMsg: public vt_dstm::BaseMessage {
	int nodeId;
	bool isResponse;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
    GroupJoinMsg() {isResponse = false;}
	GroupJoinMsg(int nodeId, bool isR);
	virtual ~GroupJoinMsg();
	static void GroupJoinHandler(HyflowMessage & msg);
	void serializationTest();

	int getNodeId() const {return nodeId;}
};

} /* namespace vt_dstm */

#endif /* GROUPJOINMSG_H_ */
