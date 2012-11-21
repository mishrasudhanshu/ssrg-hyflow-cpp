/*
 * TpccOrderLine.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCORDERLINE_H_
#define TPCCORDERLINE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccOrderLine: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & OL_O_ID;
		ar & OL_D_ID;
		ar & OL_W_ID;
		ar & OL_NUMBER;
		ar & OL_I_ID;
		ar & OL_SUPPLY_W_ID;
		ar & OL_DELIVERY_D;
		ar & OL_QUANTITY;
		ar & OL_AMOUNT;
		ar & OL_DIST_INFO;
    }
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
	TpccOrderLine() {}
	TpccOrderLine(int w_id, int d_id, int o_id, int ol_num);
	virtual ~TpccOrderLine();

	static std::string getOrderLineId(int OL_W_ID, int OL_D_ID, int OL_O_ID, int OL_NUMBER);

	void print() {}
	void getClone(HyflowObject **obj);
};

} /* namespace vt_dstm */

#endif /* TPCCORDERLINE_H_ */
