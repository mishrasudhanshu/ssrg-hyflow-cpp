/*
 * ObjectResponseMsg.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef OBJECTRESPONSEMSG_H_
#define OBJECTRESPONSEMSG_H_

#include "../HyflowMessage.h"

namespace vt_dstm {

template <class objType>
class ObjectResponseMsg: public HyflowMessage {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
public:
	objType object;

	ObjectResponseMsg();
	virtual ~ObjectResponseMsg(){}

	objType getObject(){return object;}
	void setObject(objType & obj) {object = obj;}

	void print();
};

} /* namespace vt_dstm */

#endif /* OBJECTRESPONSEMSG_H_ */
