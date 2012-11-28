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
#include "tm/tpcc/TpccBenchmark.h"
#include "../util/concurrent/HyInteger.h"

namespace vt_dstm {

class BenchmarkExecutor {
	static HyflowBenchmark *benchmark;
	static boost::thread **benchmarkThreads;

	static boost::thread_specific_ptr<HyInteger> tries;
	static boost::thread_specific_ptr<HyInteger> checkResume;
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
	static double throughPut;
	static int retryCount;
	static int checkPointResume;
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
	static void addMetaData(double trhPut, int retry, int cpResume);
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
    	TpccBenchmark::registerObjectTypes(ar);
    }

    static void executeThreads();
	static void increaseRetries();
	static void increaseCheckpoint();

	static int getCalls() {	return calls; }

	bool isSanity() const {
		return sanity;
	}
};

} /* namespace vt_dstm */

#endif /* BENCHMARKEXECUTOR_H_ */
