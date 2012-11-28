/*
 * HashMapBenchMark.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HASHTABLEBENCHMARK_H_
#define HASHTABLEBENCHMARK_H_

#include "boost/thread/tss.hpp"
#include "HashTable.h"
#include "../../HyflowBenchmark.h"
#include "../../../util/concurrent/HyInteger.h"

namespace vt_dstm {

class HashTableBenchmark: public vt_dstm::HyflowBenchmark {
	std::string* ids;
	static int objectCount;
	static int bucketCount;
	static boost::thread_specific_ptr<HyInteger> objectCreated;
public:
	HashTableBenchmark();
	virtual ~HashTableBenchmark();

	template<class Archive>
	static void registerObjectTypes(Archive & ar) {
		ar.register_type(static_cast<HashBucket*>(NULL));
	}

	int getOperandsCount();
	void readOperation(std::string ids[], int size);
	void writeOperation(std::string ids[], int size);
	void checkSanity();
	static std::string getBucketId(int key);
	std::string* createLocalObjects(int objCount);
};

} /* namespace vt_dstm */


#endif /* HASHTABLEBENCHMARK_H_ */
