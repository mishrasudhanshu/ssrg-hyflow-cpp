/*
 * TpccCustomer.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCCUSTOMER_H_
#define TPCCCUSTOMER_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccCustomer: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & C_ID;
		ar & C_D_ID;
		ar & C_W_ID;
		ar & C_FIRST;
		ar & C_MIDDLE;
		ar & C_LAST;
		ar & C_STREET_1;
		ar & C_STREET_2;
		ar & C_CITY;
		ar & C_STATE;
		ar & C_ZIP;
		ar & C_PHONE;
		ar & C_SINCE;
		ar & C_CREDIT;
		ar & C_CREDIT_LIM;
		ar & C_DISCOUNT;
		ar & C_BALANCE;
		ar & C_YTD_PAYMENT;
		ar & C_CNT_PAYMENT;
		ar & C_CNT_DELIVERY;
		ar & C_DATA_1;
		ar & C_DATA_2;
    }

public:
    int C_ID;	//		INTEGER,
    int C_D_ID;	//		INTEGER,
    int C_W_ID;	//		INTEGER,
    std::string C_FIRST; //		CHARACTER (16),
    std::string C_MIDDLE; //	CHARACTER (2),
    std::string C_LAST;	  //	VARCHAR,
    std::string C_STREET_1; //	CHARACTER (20),
    std::string C_STREET_2; //	CHARACTER (20),
    std::string C_CITY;		//  CHARACTER (20),
    std::string C_STATE;	//	CHARACTER (2),
    std::string C_ZIP;		//	CHARACTER (9),
    std::string C_PHONE;	//	CHARACTER (16),
    std::string C_SINCE;		//  VARCHAR,
    std::string C_CREDIT;	//	CHARACTER (2),
    float C_CREDIT_LIM;		//  NUMERIC,
    float C_DISCOUNT;		// NUMERIC,
    float C_BALANCE;		// NUMERIC,
    float C_YTD_PAYMENT;	// NUMERIC,
    float C_CNT_PAYMENT;	// NUMERIC,
    float C_CNT_DELIVERY;	// NUMERIC,
    std::string  C_DATA_1;		// CHARACTER (250),
    std::string  C_DATA_2;		// CHARACTER (250),

	TpccCustomer() {}
	TpccCustomer(int warehouseId, int districtId, int customerId);
	virtual ~TpccCustomer();

	void print() {}
	void getClone(HyflowObject **tCustomer);

	static std::string getCustomerId(int warehouseId, int districtId, int customerId);
	static int getRandomCustomer();
};

} /* namespace vt_dstm */

#endif /* TPCCCUSTOMER_H_ */
