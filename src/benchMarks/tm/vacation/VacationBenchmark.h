/*
 * VacationBenchmark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef VACATIONBENCHMARK_H_
#define VACATIONBENCHMARK_H_

#include <cstddef>
#include <boost/thread/tss.hpp>

#include "../../HyflowBenchmark.h"
#include "../../../util/concurrent/HyInteger.h"
#include "ReservationInfo.h"
#include "Resource.h"
#include "Customer.h"

namespace vt_dstm {
enum Actions {
	ACTION_MAKE_RESERVATION,
	ACTION_DELETE_CUSTOMER,
	ACTION_UPDATE_TABLES,
//	RESERVATION_CAR,
//	RESERVATION_FLIGHT,
//	RESERVATION_ROOM
};

class VacationBenchmark: public vt_dstm::HyflowBenchmark {
	static int queryPerTransaction;
	static int queryRange;
	static boost::thread_specific_ptr<HyInteger> objectCreated;
	std::string* ids;
	static int objectCount;
	void processRequest(Actions action);
public:
	VacationBenchmark();
	virtual ~VacationBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<Resource*>(NULL));
    	ar.register_type(static_cast<Customer*>(NULL));
    	ar.register_type(static_cast<ReservationInfo*>(NULL));
    }

	int getOperandsCount();
	static int getId();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* VACATIONBENCHMARK_H_ */
