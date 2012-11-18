/*
 * VacationBenchmark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef VACATIONBENCHMARK_H_
#define VACATIONBENCHMARK_H_

#include <cstddef>
#include "../../HyflowBenchmark.h"
#include "Reservation.h"

namespace vt_dstm {

class VacationBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	int objectCount;
public:
	VacationBenchmark();
	virtual ~VacationBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<Reservation*>(NULL));
    }

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* VACATIONBENCHMARK_H_ */
