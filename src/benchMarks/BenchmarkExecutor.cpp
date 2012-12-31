/*
 * BenchmarkExecutor.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include <cstdlib>
#include <boost/thread/thread.hpp>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "BenchmarkExecutor.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"
#include "../core/helper/RandomIdProvider.h"
#include "../util/concurrent/ThreadMeta.h"

namespace vt_dstm {

int BenchmarkExecutor::calls = 1;
int BenchmarkExecutor::delay = 0;
long BenchmarkExecutor::timeout = 0;
bool BenchmarkExecutor::checkPoint = false;
bool BenchmarkExecutor::sanity = false;
int BenchmarkExecutor::objectsCount = -1;
int BenchmarkExecutor::transactions = 0 ;
int BenchmarkExecutor::readPercent = 0;
int BenchmarkExecutor::threads = 1;
bool BenchmarkExecutor::isInitiated = false;
double BenchmarkExecutor::throughPut = 0;
int BenchmarkExecutor::threadCount = 0;
int BenchmarkExecutor::retryCount = 0;
int BenchmarkExecutor::transactionLength=0;
int BenchmarkExecutor::checkPointResume = 0;
int BenchmarkExecutor::innerTxns=1;
int BenchmarkExecutor::itcpr=1;
int BenchmarkExecutor::objectNesting=2;

std::string BenchmarkExecutor::benchMarkName;

HyflowBenchmark* BenchmarkExecutor::benchmark = NULL;
bool* BenchmarkExecutor::transactionType = NULL;
std::string*** BenchmarkExecutor::threadArgsArray = NULL;
std::string* BenchmarkExecutor::ids = NULL;
boost::thread_specific_ptr<HyInteger> BenchmarkExecutor::tries;
boost::thread_specific_ptr<HyInteger> BenchmarkExecutor::checkResume;
boost::thread** BenchmarkExecutor::benchmarkThreads = NULL;
boost::mutex BenchmarkExecutor::execMutex;

BenchmarkExecutor::BenchmarkExecutor() {}

BenchmarkExecutor::~BenchmarkExecutor() {}

unsigned long long BenchmarkExecutor::getTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}

void BenchmarkExecutor::addMetaData(double trp, int retry, int cpResume) {
	boost::unique_lock<boost::mutex> metaDatalock(execMutex);
	throughPut += trp;
	retryCount += retry;
	checkPointResume += cpResume;
}

void BenchmarkExecutor::writeResults() {
	LOG_DEBUG("Throughput=%f, retryCount=%d\n", throughPut, retryCount);
	Logger::result("Throughput=%.2f\n", throughPut);
	float abortRate = (retryCount*100)/(transactions*(innerTxns+1)*threadCount);
	Logger::result("AbortRate=%.2f\n",abortRate);
	Logger::result("CheckpointResume=%d\n",checkPointResume);
}

void BenchmarkExecutor::initExecutor(){
	if ( !isInitiated ) {
		if (ConfigFile::Value(BENCHMARK).compare(BANK) == 0) {
			benchMarkName = "Bank";
			benchmark = new BankBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(LIST) == 0) {
			benchMarkName = "List";
			benchmark = new ListBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(TEST) == 0) {
			benchMarkName = "Test";
			benchmark = new TestSpeedBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(SKIP_LIST) == 0) {
			benchMarkName = "SkipList";
			benchmark = new SkipListBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(BST) == 0) {
			benchMarkName = "Bst";
			benchmark = new BstBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(LOAN) == 0) {
			benchMarkName = "Loan";
			benchmark = new LoanBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(HASH_TABLE) == 0) {
			benchMarkName = "hashTable";
			benchmark = new HashTableBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(VACATION) == 0) {
			benchMarkName = "vacation";
			benchmark = new VacationBenchmark();
		}else if(ConfigFile::Value(BENCHMARK).compare(TPCC) == 0) {
			benchMarkName = "Tpcc";
			benchmark = new TpccBenchmark();
		}else {
			Logger::fatal("BE :Unknown Benchmark\n");
		}
		LOG_DEBUG("BE :Running %s\n", benchMarkName.c_str());
		objectsCount = atoi(ConfigFile::Value(OBJECTS).c_str());
		transactions = atoi(ConfigFile::Value(TRANSACTIONS).c_str());
		readPercent =  atoi(ConfigFile::Value(READS).c_str());
		calls = atoi(ConfigFile::Value(CALLS).c_str());
		isInitiated = true;
		threadCount = NetworkManager::getThreadCount();
		benchmarkThreads = new boost::thread*[threadCount];
		sanity = (strcmp(ConfigFile::Value(SANITY).c_str(), TRUE) == 0)? true:false;
		transactionLength = atoi(ConfigFile::Value(TRANSACTIONS_LENGTH).c_str());
		int it = atoi(ConfigFile::Value(INNER_TXNS).c_str());
		if (it>0) {
			innerTxns = it;
		}
		itcpr = atoi(ConfigFile::Value(ITCPR).c_str());
		objectNesting = atoi(ConfigFile::Value(OBJECT_NESTING).c_str());
		writeConfig();
	}
}

void BenchmarkExecutor::writeConfig() {
	Logger::result("----------%llu------------\n", getTime());
	Logger::result("Benchmark=%s\n", benchMarkName.c_str());
	Logger::result("Reads=%d%%\n", readPercent);
	Logger::result("Objects=%d\n", objectsCount);
	Logger::result("Trnxs=%d\n", transactions);
	Logger::result("Threads=%d\n",threadCount);
	Logger::result("Nodes=%d\n",NetworkManager::getNodeCount());
	Logger::result("Nesting=%s\n",ConfigFile::Value(NESTING_MODEL).c_str());
	Logger::result("InnerTxns=%d\n", innerTxns);
	Logger::result("ITCPR=%d\n", itcpr);
}

void BenchmarkExecutor::createObjects(){
	ids = benchmark->createLocalObjects(objectsCount);
}

void BenchmarkExecutor::transactionLengthDelay() {
	usleep(transactionLength*1000);
}

void BenchmarkExecutor::prepareArgs() {
	threadArgsArray = new std::string**[threadCount];
	transactionType = new bool[transactions];

	int argsCount = benchmark->getOperandsCount();
	for(int k=0 ; k < threadCount; k++) {
		threadArgsArray[k] = new std::string*[transactions];
		for (int i=0; i < transactions ; i++ ) {
			threadArgsArray[k][i] = new std::string[argsCount];
			RandomIdProvider rIdPro(objectsCount);
			for (int j=0; j < argsCount; j++ ) {
				int index = (rIdPro.getNext()) %objectsCount;
				threadArgsArray[k][i][j] = ids[index];
			}
		}
	}

	for (int i=0; i < transactions ; i++ ) {
		if ( i < (transactions*readPercent/100))
			transactionType[i] = true;
		else
			transactionType[i] = false;

	}

	// Shuffle the transaction array
	for( int k = 0; k < transactions; k++ ) {
		int r = k + (RandomIdProvider::getRandomNumber() % (transactions - k));
		bool tmp = transactionType[k];
		transactionType[k] = transactionType[r];
		transactionType[r] = tmp;
	}
}

void BenchmarkExecutor::execute(int id){
	ThreadMeta::threadInit(id, TRANSACTIONAL_THREAD);

	int argsCount = benchmark->getOperandsCount();

	LOG_DEBUG("BNCH_EXE %d:------------------------------>\n", id);
	unsigned long long start = getTime();
	std::string** argsArray = threadArgsArray[id];
	for(int i=0; i < transactions; i++) {
		int pos = (i + id) % transactions;
		if (transactionType[pos]) {
			benchmark->readOperation(argsArray[pos], argsCount);
		} else {
			benchmark->writeOperation(argsArray[pos], argsCount);
		}
	}
	unsigned long long end = getTime();
	unsigned long long executionTime = (end -start + 1);	// Get value in us
	LOG_DEBUG("Execution time %llu ms\n",executionTime/1000);
	double thrPut = (double(transactions*1000000))/executionTime;
	int rtry = 0, checkRes=0;
	if (tries.get()) {
	// Tries are also get included in retried value as we count number of contexts created
		rtry = tries.get()->getValue();
	}
	if (checkResume.get()) {
		checkRes = checkResume.get()->getValue();
	}
	addMetaData(thrPut, rtry, checkRes);
	LOG_DEBUG("BNC_EXE %d: ThroughPut = %0.2f trxns/sec <----------------------\n", id, thrPut);
//	sleep(20);
//	ThreadMeta::threadDeinit(TRANSACTIONAL_THREAD);
}

void BenchmarkExecutor::increaseRetries() {
	if (!tries.get()) {
		tries.reset(new HyInteger(0));
	}
	tries.get()->increaseValue();
	LOG_DEBUG("BE:---tries++-->%d\n", tries.get()->getValue());
}

void BenchmarkExecutor::countCheckpointResume() {
	if (!checkResume.get()) {
		checkResume.reset(new HyInteger(0));
	}
	checkResume.get()->increaseValue();
	LOG_DEBUG("BE:---Checkpoint++-->%d\n", checkResume.get()->getValue());
}
//	benchmark->warmUp();

void BenchmarkExecutor::executeThreads() {
	// Read all the configuration settings
	initExecutor();
	// Create objects and then make all nodes done populating objects
	createObjects();
	prepareArgs();
	benchmark->warmUp();

	NetworkManager::synchronizeCluster();
	sleep(2);

	int threadCount = NetworkManager::getThreadCount();
	for (int i=0; i < threadCount ; i++) {
		benchmarkThreads[i] = new boost::thread(execute, i);
	}

	for (int i=0; i < threadCount ; i++) {
		benchmarkThreads[i]->join();
	}

	writeResults();
	sleep(2);
	// Make sure all node finished transactions and then do sanity check
	NetworkManager::synchronizeCluster();
	if ( (NetworkManager::getNodeId() == 0) && (sanity) ) {
		benchmark->checkSanity();
	}
	sleep(2);	// Require to stop out of order synchronize request
	// Make sure sanity check is completed on all the nodes
	NetworkManager::synchronizeCluster();
	// DONE
}
} /* namespace vt_dstm */
