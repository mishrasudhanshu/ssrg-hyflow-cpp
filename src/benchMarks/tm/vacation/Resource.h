/*
 * Resource.h
 *
 *  Created on: Dec 5, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef RESOURCE_H_
#define RESOURCE_H_

#include <stdint.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"


namespace vt_dstm {

enum ResourceType {
	VACATION_CAR,
	VACATION_FLIGHT,
	VACATION_ROOM
};

class Resource: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & price;
    	ar & resouceType;
    }

	uint64_t price;
	ResourceType resouceType;

public:
	Resource() {};
	Resource(std::string id, uint64_t p, ResourceType t);
	~Resource();

	void print() {}
	void getClone(HyflowObject **obj);

	uint64_t getPrice() const {
		return price;
	}

	void setPrice(uint64_t price) {
		this->price = price;
	}

	ResourceType getResouceType() const {
		return resouceType;
	}

	void setResouceType(ResourceType resouceType) {
		this->resouceType = resouceType;
	}
};

} /* namespace vt_dstm */

#endif /* RESOURCE_H_ */
