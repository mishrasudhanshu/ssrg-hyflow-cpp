/*
 * BstBenchmark.h
 *
 *  Created on: Nov 16, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BSTBENCHMARK_H_
#define BSTBENCHMARK_H_

#include <vector>

#include "boost/thread/tss.hpp"
#include "BstNode.h"
#include "../../HyflowBenchmark.h"
#include "../../../util/concurrent/HyInteger.h"

namespace vt_dstm {

class BstBenchmark: public vt_dstm::HyflowBenchmark {
	int objectCount;
	std::string* ids;
	static boost::thread_specific_ptr<HyInteger> objectCreated;
	static std::vector<int> bstSequence(int start, int end, int diff);
	static void sequenceUtil(int seq[], int start, int end, int pos, int size);
public:
	BstBenchmark();
	virtual ~BstBenchmark();

	template<class Archive>
	static void registerObjectTypes(Archive & ar) {
		ar.register_type(static_cast<BstNode*>(NULL));
	}

	int getOperandsCount();
	void warmUp();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	static int getId();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* BSTBENCHMARK_H_ */
