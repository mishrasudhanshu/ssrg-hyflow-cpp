/*
 * TpccOrder.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCORDER_H_
#define TPCCORDER_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
namespace vt_dstm {

class TpccOrder: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & O_ID;
		ar & O_D_ID;
		ar & O_W_ID;
		ar & O_C_ID;
		ar & O_ENTRY_D;
		ar & O_CARRIER_ID;
		ar & O_OL_CNT;
		ar & O_ALL_LOCAL;
    }
public:
    int O_ID;	//		INTEGER,
    int O_D_ID; //		INTEGER,
    int O_W_ID; //		INTEGER,
    int O_C_ID; //		INTEGER,
    std::string O_ENTRY_D; //	DATE,
    int O_CARRIER_ID;   //	INTEGER,
    int O_OL_CNT; 		//	INTEGER,
    int O_ALL_LOCAL;	//	NUMERIC,
	TpccOrder() {};
	TpccOrder(int O_W_ID, int O_D_ID, int O_ID);
	virtual ~TpccOrder();

	static std::string getOrderId(int O_W_ID, int O_D_ID, int O_ID);
	void print() {}
	void getClone(HyflowObject **obj);
};

} /* namespace vt_dstm */

#endif /* ORDER_H_ */
