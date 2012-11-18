/*
 * HashMapBenchMark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HASHMAPBENCHMARK_H_
#define HASHMAPBENCHMARK_H_

#include "boost/thread/tss.hpp"
#include "HashMap.h"
#include "../../HyflowBenchmark.h"
#include "../../../util/concurrent/HyInteger.h"

namespace vt_dstm {

class HashMapBenchmark: public vt_dstm::HyflowBenchmark {
	static boost::thread_specific_ptr<HyInteger> objectCreated;
public:
	HashMapBenchmark();
	virtual ~HashMapBenchmark();

	template<class Archive>
	static void registerObjectTypes(Archive & ar) {
		ar.register_type(static_cast<HashMap*>(NULL));
	}

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	static int getId();
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */


#endif /* HASHMAPBENCHMARK_H_ */
