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
    	ar & ownerTrnx;
    	ar & ownerNode;
    	ar & oldOwnerNode;
    	ar & oldHyVersion;
    }
protected:
	HyflowObject(const std::string & Id, int v) {
		hyId = Id;
		oldHyVersion = hyVersion;
		hyVersion = v;
		ownerNode = -1;
		oldOwnerNode = -1;
	}

	std::string hyId;
	/*
	 * Version int size required to be 32 bit as 31st and 30st are used for
	 * Signifying remote and locking status
	 */
	int32_t hyVersion;
	int32_t oldHyVersion;

	unsigned long long ownerTrnx;
	int ownerNode;
	int oldOwnerNode;
public:

	HyflowObject() { ownerNode = -1; oldOwnerNode = -1; }
	virtual ~HyflowObject(){}

	virtual void setId(const std::string & Id) {hyId = Id;};
	virtual std::string & getId() {return hyId;};

	virtual void setVersion(int32_t v) {
		oldHyVersion = hyVersion;
		hyVersion = v;
	}

	virtual int32_t getVersion() {return hyVersion;};
	virtual void updateVersion() {hyVersion++;}
	virtual void print()=0;
	/*
	 * Provides a clone on given object created on heap, make sure to
	 * delete once removed from local cache, mostly in local cache update call
	 */
	virtual void getClone(HyflowObject **obj)=0;

	int32_t getOldHyVersion() const {
		return oldHyVersion;
	}

	int getOldOwnerNode() const {
		return oldOwnerNode;
	}

	void setOldOwnerNode(int oldOwnerNode) {
		this->oldOwnerNode = oldOwnerNode;
	}

	int getOwnerNode() const {
		return ownerNode;
	}

	void setOwnerNode(int ownerNode) {
		this->ownerNode = ownerNode;
	}

	unsigned long long getOwnerTrnx() const {
		return ownerTrnx;
	}

	void setOwnerTrnx(unsigned long long ownerTrnx) {
		this->ownerTrnx = ownerTrnx;
	}

	void baseClone(HyflowObject* obj) {
		obj->hyId = hyId;
		obj->hyVersion = hyVersion;
		obj->ownerNode = ownerNode;
		obj->oldOwnerNode = oldOwnerNode;
		obj->ownerTrnx = ownerTrnx;
		obj->oldHyVersion = oldHyVersion;
	}
};

//LESSON: Useful in case of some other type of compiler
BOOST_SERIALIZATION_ASSUME_ABSTRACT(HyflowObject)

} /* namespace vt_dstm */

#endif /* HYFLOWOBJECT_H_ */
