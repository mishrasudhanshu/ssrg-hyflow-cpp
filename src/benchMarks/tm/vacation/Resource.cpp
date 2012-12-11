/*
 * Resource.cpp
 *
 *  Created on: Dec 5, 2012
 *      Author: mishras[at]vt.edu
 */

#include "Resource.h"

namespace vt_dstm {

Resource::Resource(std::string id, uint64_t p, ResourceType t) {
	hyId = id;
	price = p;
	resouceType = t;
}

Resource::~Resource() {
	// TODO Auto-generated destructor stub
}

void Resource::getClone(HyflowObject **obj){
	Resource *rs = new Resource();
	rs->price = price;
	rs->resouceType = resouceType;
	this->baseClone(rs);
	*obj = rs;
}

} /* namespace vt_dstm */
