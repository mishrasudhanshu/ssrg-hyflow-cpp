/*
 * Customer.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "Customer.h"
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../core/helper/CheckPointProvider.h"

namespace vt_dstm {

Customer::Customer(const std::string & Id)
{
	hyId = Id;
	hyVersion = 0;

	//TODO: Add register object call in constructor itself: Requires
	// thread local storage of tid, if not defined register, else add to
	// lazy publish set of object
}

Customer::Customer(const std::string & Id, int v)
{
	hyId = Id;
	hyVersion = v;
}

void Customer::addReseverationInfo(ReservationInfo* resInfo)
{
	reservations.push_back(resInfo->getId());
}

Customer::~Customer() {}

void Customer::print(){}

void Customer::getClone(HyflowObject **obj){
	Customer *ba = new Customer();
	ba->reservations = reservations;
	this->baseClone(ba);
	*obj = ba;
}

void Customer::checkSanity(std::string* ids, int objectCount) {}

// Serialisation Test of object
void Customer::test() {}

} /* namespace vt_dstm */
