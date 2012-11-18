/*
 * TCustomer.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TCUSTOMER_H_
#define TCUSTOMER_H_

#include "HyflowObject.h"

namespace vt_dstm {

class TCustomer: public vt_dstm::HyflowObject {
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

	TCustomer();
	virtual ~TCustomer();

	void print() {}
	void getClone(HyflowObject **tCustomer);

	static std::string getCustomerId(int warehouseId, int districtId, int customerId);
	static int getRandomCustomer();
};

} /* namespace vt_dstm */

#endif /* TCUSTOMER_H_ */
