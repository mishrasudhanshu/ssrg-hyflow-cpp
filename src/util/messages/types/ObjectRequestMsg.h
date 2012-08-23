/*
 * ObjRequestMsg.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef OBJECTREQUESTMSG_H_
#define OBJECTREQUESTMSG_H_

#include <string>
#include "../HyflowMessage.h"

namespace vt_dstm {

class ObjectRequestMsg: public HyflowMessage {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	std::string id;
	bool isRead;

	ObjectRequestMsg(){isRead = true;}
	ObjectRequestMsg(std::string id, bool isRead);
	virtual ~ObjectRequestMsg();

	void print();
};

} /* namespace vt_dstm */

#endif /* OBJECTREQUESTMSG_H_ */
