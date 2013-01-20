/*
 * BenchmarkExecutor.h
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BENCHMARKEXECUTOR_H_
#define BENCHMARKEXECUTOR_H_

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
	HYFLOW_METADATA_RUNTIME,
	HYFLOW_METADATA_COMMITED_TXNS,
	HYFLOW_METADATA_COMMITED_SUBTXNS,
	HYFLOW_METADATA_COMMITED_SUBTXN_TIME,
	HYFLOW_METADATA_ABORTED_SUBTXNS,
	HYFLOW_METADATA_ABORTED_SUBTXN_TIME,
	HYFLOW_METADATA_COMPENSATE_SUBTXNS,
	HYFLOW_METADATA_COMPENSATE_SUBTXN_TIME,
	HYFLOW_METADATA_BACKOFF_TIME,
	// Add New Types above
	HYFLOW_METADATA_ALL,
};

class HyflowMetaData{
public:
	HyInteger txnTries;
	HyInteger txnAborts;
	HyInteger txnCheckpointResume;
	unsigned long long txnRunTime;
	unsigned int committedTxns;

	unsigned int committedSubTxns;
	unsigned long long committedSubTxnTime;

	unsigned int abortedSubTxns;
	unsigned long long abortedSubTxnTime;

	unsigned int compensateSubTxns;
	unsigned long long compensateSubTxnTime;
	unsigned long long backOffTime;

	HyflowMetaData() {
		txnTries = 0;
		txnAborts = 0;
		txnCheckpointResume = 0;
		txnRunTime = 0;
		committedTxns = 0;

		committedSubTxns = 0;
		committedSubTxnTime = 0;
		abortedSubTxns = 0;
		abortedSubTxnTime = 0;
		compensateSubTxns = 0;
		compensateSubTxnTime = 0;
		backOffTime = 0;
	}
	void increaseMetaData(HyflowMetaDataType type);
	void updateMetaData(HyflowMetaData& metadata, HyflowMetaDataType type);
};

class BenchmarkExecutor {
	static HyflowBenchmark *benchmark;
	static pthread_t *benchmarkThreads;
	static HyflowMetaData* benchMarkThreadMetadata;

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
	static unsigned long executionTime;

	static boost::mutex* metaWriteMutex;
	static bool* transactionType;
	static std::string*** threadArgsArray;
	static std::string* ids;
	static std::string benchMarkName;


	static void prepareArgs();

	static unsigned long long getTime();
	static void writeResults();
	static std::string& randomId();
	static void createObjects();
    static void* execute(void* id);
    static void executeTransaction(int txnId, int threadId);
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

    /*
     * Returns true if all threads completed within timeout
     */
    static bool executeThreads();

	static void updateMetaData(HyflowMetaData data, HyflowMetaDataType type);
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
