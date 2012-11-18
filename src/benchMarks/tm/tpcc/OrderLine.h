/*
 * OrderLine.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ORDERLINE_H_
#define ORDERLINE_H_

#include "HyflowObject.h"

namespace vt_dstm {

class OrderLine: public vt_dstm::HyflowObject {
public:
	int OL_O_ID; 		//Order Line order Id		INTEGER,
	int OL_D_ID;		//Order Line District Id	INTEGER,
	int OL_W_ID;		//Order Line Warehouse Id	INTEGER,
	int OL_NUMBER;		//Order Line number			INTEGER,
	int OL_I_ID;		//Order Line Iterm number	INTEGER,
	int OL_SUPPLY_W_ID;	//Order Line Supply Warehouse INTEGER,
	int OL_DELIVERY_D;	//Order Line Delivery	DATE,
	float OL_QUANTITY;	//Order Line quantity of objects NUMERIC,
	float OL_AMOUNT;	//Order Line cost of objects NUMERIC,
	std::string OL_DIST_INFO;	//Order Line District Info CHARACTER (24),
	OrderLine();
	OrderLine(int itemId, int quantity, int supplyWarehouseId);
	virtual ~OrderLine();

	static std::string getOrderLineId(int OL_W_ID, int OL_D_ID, int OL_O_ID, int OL_NUMBER);

	void print() {}
	void getClone(HyflowObject **obj);
};

} /* namespace vt_dstm */

#endif /* ORDERLINE_H_ */
