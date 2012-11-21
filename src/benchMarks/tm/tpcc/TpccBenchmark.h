/*
 * TpccBenchmark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCBENCHMARK_H_
#define TPCCBENCHMARK_H_

#include <cstddef>
#include "../../HyflowBenchmark.h"
#include "TpccDistrict.h"
#include "TpccItem.h"
#include "TpccNewOrder.h"
#include "TpccOrder.h"
#include "TpccOrderLine.h"
#include "TpccStock.h"
#include "TpccCustomer.h"
#include "TpccWareHouse.h"
#include "boost/thread/tss.hpp"
#include "../../../util/concurrent/HyInteger.h"

namespace vt_dstm {

class TpccBenchmark: public vt_dstm::HyflowBenchmark {
	static boost::thread_specific_ptr<HyInteger> historyCreated;
	std::string* ids;
	int objectCount;

	void TpccOperation();
public:
	TpccBenchmark();
	virtual ~TpccBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<TpccDistrict*>(NULL));
    	ar.register_type(static_cast<TpccItem*>(NULL));
    	ar.register_type(static_cast<TpccNewOrder*>(NULL));
    	ar.register_type(static_cast<TpccOrder*>(NULL));
    	ar.register_type(static_cast<TpccOrderLine*>(NULL));
    	ar.register_type(static_cast<TpccStock*>(NULL));
    	ar.register_type(static_cast<TpccCustomer*>(NULL));
    	ar.register_type(static_cast<TpccWareHouse*>(NULL));
    }

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	static int getOrderBase(int district);
	static int getNewOrderBase(int district);
	static int getOrderLineBase(int district);
	static int getNextHistory();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* TPCCBENCHMARK_H_ */
