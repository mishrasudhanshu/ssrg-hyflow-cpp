/*
 * SynchronizeMsg.h
 * This message synchronize the whole cluster around node 0.
 * TODO: Fix out of order synchronize message issue
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef SYNCHRONIZEMSG_H_
#define SYNCHRONIZEMSG_H_

#include "../BaseMessage.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

class SynchronizeMsg: public vt_dstm::BaseMessage {
	int nodeId;
	int requestNo;
	bool isResponse;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
    SynchronizeMsg() {isResponse = false;}
	SynchronizeMsg(int nodeId, bool isR, int rn);
	virtual ~SynchronizeMsg();
	static void synchronizeHandler(HyflowMessage & msg);
	void serializationTest();

	int getNodeId() const {return nodeId;}
};

} /* namespace vt_dstm */

#endif /* SYNCHRONIZEMSG_H_ */
