/*
 * TPCCBenchmark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCBENCHMARK_H_
#define TPCCBENCHMARK_H_

#include <cstddef>
#include "../../HyflowBenchmark.h"
#include "AuxShipped.h"
#include "District.h"
#include "Item.h"
#include "NewOrder.h"
#include "Order.h"
#include "OrderLine.h"
#include "Stock.h"
#include "TCustomer.h"
#include "TPCCWareHouse.h"

namespace vt_dstm {

class TPCCBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	int objectCount;
	void TpccOperation();
public:
	TPCCBenchmark();
	virtual ~TPCCBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<AuxShipped*>(NULL));
    	ar.register_type(static_cast<District*>(NULL));
    	ar.register_type(static_cast<Item*>(NULL));
    	ar.register_type(static_cast<NewOrder*>(NULL));
    	ar.register_type(static_cast<Order*>(NULL));
    	ar.register_type(static_cast<OrderLine*>(NULL));
    	ar.register_type(static_cast<Stock*>(NULL));
    	ar.register_type(static_cast<TCustomer*>(NULL));
    	ar.register_type(static_cast<TPCCWareHouse*>(NULL));
    }

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* TPCCBENCHMARK_H_ */
