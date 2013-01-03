/*
 * BenchmarkExecutor.h
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BENCHMARKEXECUTOR_H_
#define BENCHMARKEXECUTOR_H_

#include <boost/thread/tss.hpp>
#include "HyflowBenchmark.h"
#include "tm/bank/BankBenchmark.h"
#include "tm/list/ListBenchmark.h"
#include "tm/test/TestSpeedBenchmark.h"
#include "tm/skipList/SkipListBenchmark.h"
#include "tm/bst/BstBenchmark.h"
#include "tm/loan/LoanBenchMark.h"
#include "tm/hashTable/HashTableBenchMark.h"
#include "tm/vacation/VacationBenchmark.h"
#include "tm/tpcc/TpccBenchmark.h"
#include "../util/concurrent/HyInteger.h"

namespace vt_dstm {

enum HyflowMetaDataType{
	HYFLOW_METADATA_TRIES,
	HYFLOW_METADATA_ABORTS,
	HYFLOW_METADATA_CHECKPOINT_RESUME,
	HYFLOW_METADATA_THROUGHPUT,
	// Add New Types above
	HYFLOW_METADATA_ALL,
};

class HyflowMetaData{
public:
	HyInteger txnTries;
	HyInteger txnAborts;
	HyInteger txnCheckpointResume;
	double throughPut;
	HyflowMetaData() {
		txnTries = 0;
		txnAborts = 0;
		txnCheckpointResume = 0;
		throughPut = 0;
	}
	void increaseMetaData(HyflowMetaDataType type);
	void updateMetaData(HyflowMetaData& metadata, HyflowMetaDataType type);
};

class BenchmarkExecutor {
	static HyflowBenchmark *benchmark;
	static boost::thread **benchmarkThreads;

	static boost::thread_specific_ptr<HyflowMetaData> benchMarkThreadMetadata;

	static int calls;
	static int delay;
	static long timeout;
	static bool checkPoint;
	static int objectsCount;
	static int transactions;
	static int readPercent;
	static int threads;
	static bool isInitiated;
	static bool sanity;
	static int threadCount;
	static HyflowMetaData benchNodeMetadata;
	static int transactionLength;
	static int innerTxns;
	static int itcpr;
	static int objectNesting;
	static boost::mutex execMutex;

	static bool* transactionType;
	static std::string*** threadArgsArray;
	static std::string* ids;
	static std::string benchMarkName;


	static void prepareArgs();

	static unsigned long long getTime();
	static void writeResults();
	static std::string& randomId();
	static void createObjects();
    static void execute(int id);
	static void submitThreadMetaData(HyflowMetaData& threadMetadata);
	static void writeConfig();
public:
	BenchmarkExecutor();
	virtual ~BenchmarkExecutor();

	static void initExecutor();

	/* TODO: Try to retrieve this using configuration file
	 * This function allows user to register its class so that it can be
	 * registered as HyflowObject derived class in boost::serialization
	 */
    template<class Archive>
	static void registerObjectTypes(Archive & ar) {
    	BankBenchmark::registerObjectTypes(ar);
    	ListBenchmark::registerObjectTypes(ar);
    	TestSpeedBenchmark::registerObjectTypes(ar);
    	SkipListBenchmark::registerObjectTypes(ar);
    	BstBenchmark::registerObjectTypes(ar);
    	LoanBenchmark::registerObjectTypes(ar);
    	HashTableBenchmark::registerObjectTypes(ar);
    	VacationBenchmark::registerObjectTypes(ar);
    	TpccBenchmark::registerObjectTypes(ar);
    }

    static void executeThreads();

	static void increaseMetaData(HyflowMetaDataType type);

	static void transactionLengthDelay();

	static int getItcpr(){
		return itcpr;
	}

	static int getObjectNesting() {
		return objectNesting;
	}

	static int getInnerTxns(){
		return innerTxns;
	}

	static int getCalls() {	return calls; }

	bool isSanity() const {
		return sanity;
	}
};

} /* namespace vt_dstm */

#endif /* BENCHMARKEXECUTOR_H_ */
