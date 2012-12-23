/*
 * HyflowBenchmark.h
 *	This is base benchmark class which should be overloaded by
 *	every new benchmark.
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWBENCHMARK_H_
#define HYFLOWBENCHMARK_H_

#include <string>

namespace vt_dstm {

class HyflowBenchmark {

public:
	HyflowBenchmark(){}
	virtual ~HyflowBenchmark(){}

	virtual int getOperandsCount() = 0;
	virtual void warmUp() {}
	virtual void readOperation(std::string ids[], int size) = 0;
	virtual void writeOperation(std::string ids[], int size) = 0;
	virtual void checkSanity() = 0;
	virtual std::string* createLocalObjects(int objectCount) = 0;
};

} /* namespace vt_dstm */

#endif /* HYFLOWBENCHMARK_H_ */
