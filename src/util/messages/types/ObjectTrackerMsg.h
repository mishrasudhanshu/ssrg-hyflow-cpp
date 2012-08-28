/*
 * ObjectTrackerMsg.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef OBJECTTRACKERMSG_H_
#define OBJECTTRACKERMSG_H_

#include <string>
#include "../BaseMessage.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

class ObjectTrackerMsg: public vt_dstm::BaseMessage {
	std::string objectId;
	int nodeId;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
    ObjectTrackerMsg(){};
	ObjectTrackerMsg(std::string objId);
	ObjectTrackerMsg(std::string objId, int id);
	virtual ~ObjectTrackerMsg();

	std::string & getObjectId() {return objectId;}
	static void objectTrackerHandler(HyflowMessage & msg);
	void serializationTest();
};

} /* namespace vt_dstm */

#endif /* OBJECTTRACKERMSG_H_ */
