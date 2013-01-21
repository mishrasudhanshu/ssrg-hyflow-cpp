/*
 * SkipListBenchmark.h
 *
 *  Created on: Nov 15, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef SKIPLISTBENCHMARK_H_
#define SKIPLISTBENCHMARK_H_

#include "boost/thread/tss.hpp"
#include "SkipListNode.h"
#include "../../HyflowBenchmark.h"
#include "../../../util/concurrent/HyInteger.h"

namespace vt_dstm {

class SkipListBenchmark: public vt_dstm::HyflowBenchmark {
	int objectCount;
	std::string* ids;
	static boost::thread_specific_ptr<HyInteger> objectCreated;
	static int skipListLevels;
public:
	SkipListBenchmark();
	virtual ~SkipListBenchmark();

	template<class Archive>
	static void registerObjectTypes(Archive & ar) {
		ar.register_type(static_cast<SkipListNode*>(NULL));
	}

	int getOperandsCount();
	void warmUp();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	static int getId();
	std::string* createLocalObjects(int objCount);

	static int getSkipListLevels() {
		return skipListLevels;
	}
};

} /* namespace vt_dstm */


#endif /* SKIPLISTBENCHMARK_H_ */
