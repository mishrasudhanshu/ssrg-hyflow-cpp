/*
 * ListBenchmark.h
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LISTBENCHMARK_H_
#define LISTBENCHMARK_H_

#include "boost/thread/tss.hpp"
#include "ListNode.h"
#include "../../HyflowBenchmark.h"
#include "../../../util/concurrent/HyInteger.h"

namespace vt_dstm {

class ListBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	static boost::thread_specific_ptr<HyInteger> objectCreated;
public:
	ListBenchmark();
	virtual ~ListBenchmark();

	template<class Archive>
	static void registerObjectTypes(Archive & ar) {
		ar.register_type(static_cast<ListNode*>(NULL));
	}

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	static int getId();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */

#endif /* LISTBENCHMARK_H_ */
