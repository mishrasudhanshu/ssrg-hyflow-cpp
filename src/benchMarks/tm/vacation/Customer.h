/*
 * Customer.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CUSTOMER_H_
#define CUSTOMER_H_
#include <stdint.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "ReservationInfo.h"

#define AMOUNT 100000

namespace vt_dstm {

class CustomerArgs {
public:
	int money;
	std::string id1;
	std::string id2;

	CustomerArgs(int m, std::string i1, std::string i2) {
		money= m;
		id1 = i1;
		id2 = i2;
	}
};

class Customer: public HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & reservations;
    }

	std::vector<std::string> reservations;

public:
	Customer() {};
	Customer(const std::string & Id);
	Customer(const std::string & Id, int version);
	virtual ~Customer();

	void addReseverationInfo(ReservationInfo* resInfo);
	void print();
	void getClone(HyflowObject **obj);
	static void checkSanity(std::string* ids, int objectCount);
	void test();
};

} /* namespace vt_dstm */
#endif /* CUSTOMER_H_ */
