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

namespace vt_dstm {

class TPCCBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	int objectCount;
public:
	TPCCBenchmark();
	virtual ~TPCCBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<TPCC*>(NULL));
    }

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* TPCCBENCHMARK_H_ */
