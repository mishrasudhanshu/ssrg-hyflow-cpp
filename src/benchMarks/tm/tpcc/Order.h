/*
 * Order.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ORDER_H_
#define ORDER_H_

#include "HyflowObject.h"

namespace vt_dstm {

class Order: public vt_dstm::HyflowObject {
public:
    int O_ID;	//		INTEGER,
    int O_D_ID; //		INTEGER,
    int O_W_ID; //		INTEGER,
    int O_C_ID; //		INTEGER,
    std::string O_ENTRY_D; //	DATE,
    int O_CARRIER_ID;   //	INTEGER,
    int O_OL_CNT; 		//	INTEGER,
    int O_ALL_LOCAL;	//	NUMERIC,
	Order();
	Order(int O_W_ID, int O_D_ID, int O_ID) {
		this->O_W_ID = O_W_ID;
		this->O_D_ID = O_D_ID;
		this->O_ID = O_ID;
	}
	virtual ~Order();

	static std::string getOrderId(int O_W_ID, int O_D_ID, int O_ID);
	void print() {}
	void getClone(HyflowObject **obj);
};

} /* namespace vt_dstm */

#endif /* ORDER_H_ */
