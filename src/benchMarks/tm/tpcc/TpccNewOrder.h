/*
 * TpccNewOrder.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCNEWORDER_H_
#define TPCCNEWORDER_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccNewOrder: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & NO_O_ID;
    	ar & NO_D_ID;
    	ar & NO_W_ID;
    }
public:
    int NO_O_ID;		// INTEGER,
    int NO_D_ID;	 	// INTEGER,
    int NO_W_ID;		// INTEGER,

	TpccNewOrder() {}
	TpccNewOrder(int NO_W_ID, int NO_D_ID, int NO_O_ID);

	virtual ~TpccNewOrder();

    static std::string getNewOrderId(int NO_W_ID, int NO_D_ID, int NO_O_ID);

    void print() {}
    void getClone(HyflowObject** obj);
};

} /* namespace vt_dstm */

#endif /* NEWORDER_H_ */
