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

namespace vt_dstm {

class BenchmarkExecutor {
	static HyflowBenchmark *benchmark;

	static int calls;
	static int delay;
	static long timeout;
	static bool checkPoint;
	static int objectsCount;
	static int transactions;
	static int readPercent;
	static int threads;
	static unsigned long long executionTime;
	static bool isInitiated;

	static bool* transactionType;
	static std::string** argsArray;
	static std::string* ids;


	static void prepareArgs();

	static unsigned long long getTime();
	static void writeResults();
	static std::string& randomId();
	static void createObjects();
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
    }

    static void execute();

};

} /* namespace vt_dstm */

#endif /* BENCHMARKEXECUTOR_H_ */
