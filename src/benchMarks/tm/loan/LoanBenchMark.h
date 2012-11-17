/*
 * LoanBenchMark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOANBENCHMARK_H_
#define LOANBENCHMARK_H_

#include "LoanAccount.h"
#include <cstddef>
#include "../../HyflowBenchmark.h"

namespace vt_dstm {

class LoanBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	int objectCount;
public:
	LoanBenchmark();
	virtual ~LoanBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<LoanAccount*>(NULL));
    }

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* LOANBENCHMARK_H_ */
