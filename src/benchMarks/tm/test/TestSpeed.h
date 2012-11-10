/*
 * TestSpeed.h
 * This class is create verify the efficiency of Networking Library
 *  Created on: Nov 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TESTSPEED_H_
#define TESTSPEED_H_

#include "../../HyflowBenchmark.h"

namespace vt_dstm {

class TestSpeed: public vt_dstm::HyflowBenchmark {
public:
	TestSpeed();
	virtual ~TestSpeed();

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

#endif /* TESTSPEED_H_ */
