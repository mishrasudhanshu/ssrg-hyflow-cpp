/*
 * AbstractObject.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWOBJECT_H_
#define HYFLOWOBJECT_H_

#include <string>
#include <iostream>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/access.hpp>

namespace vt_dstm {

class HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & hyId;
    	ar & hyVersion;
    }
protected:
	std::string hyId;
	int hyVersion;

	HyflowObject(const std::string & Id, int v)
		: hyId(Id), hyVersion(v) {}
public:

	HyflowObject() {}
	virtual ~HyflowObject(){}

	virtual void setId(const std::string & Id) {hyId = Id;};
	virtual std::string & getId() {return hyId;};

	virtual void setVersion(int v) {hyVersion = v;};
	virtual int getVersion() {return hyVersion;};
	virtual void print(){};
	virtual void getClone(HyflowObject **obj){std::cerr<<"Hyflow base class clone function called!!\n";}
};

//LESSON: Useful in case of some other type of compiler
BOOST_SERIALIZATION_ASSUME_ABSTRACT(HyflowObject)

} /* namespace vt_dstm */

#endif /* HYFLOWOBJECT_H_ */
