/*
 * BankBenchmark.h
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BANKBENCHMARK_H_
#define BANKBENCHMARK_H_

#include <cstddef>
#include "BankAccount.h"
#include "../../HyflowBenchmark.h"

namespace vt_dstm {

class BankBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	int objectCount;
public:
	BankBenchmark();
	virtual ~BankBenchmark();

    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	ar.register_type(static_cast<BankAccount*>(NULL));
    }

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* BANKBENCHMARK_H_ */
