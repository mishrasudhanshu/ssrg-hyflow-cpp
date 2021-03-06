/*
 * ObjRequestMsg.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef OBJECTACCESSMSG_H_
#define OBJECTACCESSMSG_H_

#include <string>
#include <cstddef>
#include "../BaseMessage.h"
#include "../../../core/HyflowObject.h"
#include "../HyflowMessage.h"

namespace vt_dstm {

class ObjectAccessMsg: public BaseMessage {
	std::string id;
	bool isRead;
	HyflowObject* object;

	friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	ObjectAccessMsg(){isRead = true; object = NULL;}
	ObjectAccessMsg(std::string id, bool isRead);
	virtual ~ObjectAccessMsg(){
		if (object) {
			HyflowObject *saveObject = object;
			object = NULL;
			delete saveObject;
		}
	}

	HyflowObject * getObject(){return object;}
	void setObject(HyflowObject * obj) {object = obj;}

	std::string & getId() {return id;}
	void print();
	void serializationTest();

	static void objectAccessHandler(HyflowMessage & msg);
};

} /* namespace vt_dstm */

#endif /* OBJECTREQUESTMSG_H_ */
