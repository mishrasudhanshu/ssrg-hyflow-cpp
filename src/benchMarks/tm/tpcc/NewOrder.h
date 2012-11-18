/*
 * NewOrder.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef NEWORDER_H_
#define NEWORDER_H_

#include "HyflowObject.h"

namespace vt_dstm {

class NewOrder: public vt_dstm::HyflowObject {
public:
    int NO_O_ID;		// INTEGER,
    int NO_D_ID;	 	// INTEGER,
    int NO_W_ID;		// INTEGER,

	NewOrder();
	NewOrder(int NO_W_ID, int NO_D_ID, int NO_O_ID) {
		this->NO_W_ID = NO_W_ID;
		this->NO_D_ID = NO_D_ID;
		this->NO_O_ID = NO_O_ID;
	}

	virtual ~NewOrder();

    static std::string getNewOrderId(int NO_W_ID, int NO_D_ID, int NO_O_ID);

    void print() {}
    void getClone(HyflowObject** obj);
};

} /* namespace vt_dstm */

#endif /* NEWORDER_H_ */
