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
#include <sched.h>
#include "BenchmarkExecutor.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"
#include "../core/helper/RandomIdProvider.h"
#include "../util/concurrent/ThreadId.h"

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

HyflowBenchmark* BenchmarkExecutor::benchmark = NULL;
bool* BenchmarkExecutor::transactionType = NULL;
std::string** BenchmarkExecutor::argsArray = NULL;
std::string* BenchmarkExecutor::ids = NULL;
boost::thread_specific_ptr<HyInteger> BenchmarkExecutor::retries;
boost::thread** BenchmarkExecutor::benchmarkThreads = NULL;
boost::mutex BenchmarkExecutor::execMutex;

BenchmarkExecutor::BenchmarkExecutor() {}

BenchmarkExecutor::~BenchmarkExecutor() {}

unsigned long long BenchmarkExecutor::getTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}

void BenchmarkExecutor::addMetaData(double trp, int retry) {
	boost::unique_lock<boost::mutex> metaDatalock(execMutex);
	throughPut += trp;
	retryCount += retry;
}

void BenchmarkExecutor::writeResults() {
	Logger::result("Throughput=%.2f\n", throughPut);
	Logger::result("Retries=%d\n",retryCount);
}

void BenchmarkExecutor::initExecutor(){
	if ( !isInitiated ) {
		benchmark = new BankBenchmark();
		objectsCount = atoi(ConfigFile::Value(OBJECTS).c_str());
		transactions = atoi(ConfigFile::Value(TRANSACTIONS).c_str());
		readPercent =  atoi(ConfigFile::Value(READS).c_str());
		isInitiated = true;
		threadCount = NetworkManager::getThreadCount();
		benchmarkThreads = new boost::thread*[threadCount];
		sanity = (strcmp(ConfigFile::Value(SANITY).c_str(), TRUE) == 0)? true:false;
		writeConfig();
	}
}

void BenchmarkExecutor::writeConfig() {
	Logger::result("----------%llu------------\n", getTime());
	Logger::result("Benchmark=bank\n");
	Logger::result("Reads=%d%%\n", readPercent);
	Logger::result("Objects=%d\n", objectsCount);
	Logger::result("Trnxs=%d\n", transactions);
	Logger::result("Threads=%d\n",threadCount);
	Logger::result("Nodes=%d\n",NetworkManager::getNodeCount());
}

void BenchmarkExecutor::createObjects(){
	ids = benchmark->createLocalObjects(objectsCount);
}

void BenchmarkExecutor::prepareArgs() {
	argsArray = new std::string*[transactions];
	transactionType = new bool[transactions];

	int argsCount = benchmark->getOperandsCount();
	for (int i=0; i < transactions ; i++ ) {
		argsArray[i] = new std::string[argsCount];
		RandomIdProvider rIdPro(objectsCount);
		for (int j=0; j < argsCount; j++ ) {
			int index = (rIdPro.getNext()) %objectsCount;
			argsArray[i][j] = ids[index];
		}
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
	// Set the thread affinity
	cpu_set_t s;
	CPU_ZERO(&s);
	int node = NetworkManager::getNodeId()*threadCount + id;
	CPU_SET(node, &s);
	sched_setaffinity(0, sizeof(cpu_set_t), &s);

	ThreadId::setThreadId(id);
	int argsCount = benchmark->getOperandsCount();
	LOG_DEBUG("BNCH_EXE %d:------------------------------>\n", id);
	unsigned long long start = getTime();
	for(int i=0; i < transactions; i++) {
		int pos = (i + id) % transactions;
		if (transactionType[i]) {
			benchmark->readOperation(argsArray[pos], argsCount);
		} else {
			benchmark->writeOperation(argsArray[pos], argsCount);
		}
	}
	unsigned long long end = getTime();
	unsigned long long executionTime = (end -start + 1);	// Get value in us
	LOG_DEBUG("Execution time %llu ms\n",executionTime/1000);
	double thrPut = (transactions*1000000)/executionTime;
	int rtry = 0;
	if (retries.get()) {
		rtry = retries.get()->getValue();
	}
	addMetaData(thrPut, rtry);
	LOG_DEBUG("BNC_EXE %d: ThroughPut = %0.2f trxns/sec <----------------------\n", id, thrPut);
}

void BenchmarkExecutor::increaseRetries() {
	if (!retries.get()) {
		retries.reset(new HyInteger(0));
	}
	retries.get()->increaseValue();
}

void BenchmarkExecutor::executeThreads() {
	// Read all the configuration settings
	initExecutor();
	// Create objects and then make all nodes done populating objects
	createObjects();
	prepareArgs();

	NetworkManager::synchronizeCluster(2);
	sleep(2);

	int threadCount = NetworkManager::getThreadCount();
	for (int i=0; i < threadCount ; i++) {
		benchmarkThreads[i] = new boost::thread(execute, i);
	}

	for (int i=0; i < threadCount ; i++) {
		benchmarkThreads[i]->join();
	}

	writeResults();
	// Make sure all node finished transactions and then do sanity check
	NetworkManager::synchronizeCluster(3);
	if ( (NetworkManager::getNodeId() == 0) && (sanity) ) {
		benchmark->checkSanity();
	}
	// Make sure sanity check is completed on all the nodes
	NetworkManager::synchronizeCluster(4);
	// DONE
}
} /* namespace vt_dstm */
