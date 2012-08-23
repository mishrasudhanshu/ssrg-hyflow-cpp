/*
 * AbstractObject.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWOBJECT_H_
#define HYFLOWOBJECT_H_

#include <string>
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
	HyflowObject(const std::string & Id, int v)
		: hyId(Id), hyVersion(v) {}
public:
	std::string hyId;
	int hyVersion;

	HyflowObject() {}
	virtual ~HyflowObject(){}

	void setId(const std::string & Id) {hyId = Id;};
	std::string getId() {return hyId;};

	void setVersion(int v) {hyVersion = v;};
	int getVersion() {return hyVersion;};
};

//LESSON: Useful in case of some other type of compiler
BOOST_SERIALIZATION_ASSUME_ABSTRACT(HyflowObject)

} /* namespace vt_dstm */

#endif /* HYFLOWOBJECT_H_ */
