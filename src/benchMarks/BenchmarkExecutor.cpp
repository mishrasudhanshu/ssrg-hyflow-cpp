/*
 * BenchmarkExecutor.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include <cstdlib>
#include <boost/thread/thread.hpp>
#include "BenchmarkExecutor.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"
#include "../core/helper/RandomIdProvider.h"

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
int BenchmarkExecutor::executionTime = 0;
int BenchmarkExecutor::threadCount = 0;

HyflowBenchmark* BenchmarkExecutor::benchmark = NULL;
bool* BenchmarkExecutor::transactionType = NULL;
std::string** BenchmarkExecutor::argsArray = NULL;
std::string* BenchmarkExecutor::ids = NULL;
boost::thread_specific_ptr<Integer> BenchmarkExecutor::threadId;
boost::thread** BenchmarkExecutor::benchmarkThreads = NULL;
boost::mutex BenchmarkExecutor::execMutex;

BenchmarkExecutor::BenchmarkExecutor() {}

BenchmarkExecutor::~BenchmarkExecutor() {}

unsigned long long BenchmarkExecutor::getTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + 0.001*tv.tv_usec;
}

void BenchmarkExecutor::addExecTime(unsigned long long time) {
	boost::unique_lock<boost::mutex> execlock(execMutex);
	executionTime += time;
}

void BenchmarkExecutor::writeResults() {
	double trp =  (threadCount*transactions*1000)/executionTime;
	Logger::result("Throughput=%.2f\n", trp);
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
	int nodeId = NetworkManager::getNodeId();

	int argsCount = benchmark->getOperandsCount();
	for (int i=0; i < transactions ; i++ ) {
		argsArray[i] = new std::string[argsCount];
		RandomIdProvider rIdPro(objectsCount);
		for (int j=0; j < argsCount; j++ ) {
			int index = (rIdPro.getNext() + nodeId + i) %objectsCount;
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
	threadId.reset(new Integer(id));

	int argsCount = benchmark->getOperandsCount();
	LOG_DEBUG("BNCH_EXE %d:------------------------------>\n", id);
	unsigned long long startTime = getTime();
	for(int i=0; i < transactions; i++) {
		int pos = (i + id) % transactions;
		if (transactionType[i]) {
			benchmark->readOperation(argsArray[pos], argsCount);
		} else {
			benchmark->writeOperation(argsArray[pos], argsCount);
		}
	}
	unsigned long long executionTime = getTime() + 1 - startTime;
	addExecTime(executionTime);
	LOG_DEBUG("BNC_EXE %d: Execution time = %llu msec <----------------------\n", id, executionTime);
}

int BenchmarkExecutor::getThreadId(){
	if (!threadId.get()) {
		threadId.reset(new Integer(0));
	}
	return threadId.get()->getValue();
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
