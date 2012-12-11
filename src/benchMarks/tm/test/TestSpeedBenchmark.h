/*
 * TestSpeedBenchmark.h
 * This class is created to verify the efficiency of Networking Library
 *  Created on: Nov 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TESTSPEEDBENCHMARK_H_
#define TESTSPEEDBENCHMARK_H_

#include "../../HyflowBenchmark.h"

namespace vt_dstm {

class TestSpeedBenchmark: public vt_dstm::HyflowBenchmark {
public:
	TestSpeedBenchmark();
	virtual ~TestSpeedBenchmark();

	template<class Archive>
	static void registerObjectTypes(Archive & ar) {
		// We don't create any type of object
	}

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	std::string* createLocalObjects(int objectCount);
};

} /* namespace vt_dstm */

#endif /* TESTSPEEDBENCHMARK_H_ */
