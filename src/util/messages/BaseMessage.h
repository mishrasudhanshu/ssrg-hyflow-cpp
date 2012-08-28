/*
 * BaseMessage.h
 *
 *  Created on: Aug 23, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BASEMESSAGE_H_
#define BASEMESSAGE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace vt_dstm {

class BaseMessage {
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){}
public:
	BaseMessage(){};
	virtual ~BaseMessage(){};
	virtual void serializationTest(){};
};

} /* namespace vt_dstm */

#endif /* BASEMESSAGE_H_ */
