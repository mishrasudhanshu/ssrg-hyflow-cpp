/*
 * BenchmarkExecutor.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include <cstdlib>
#include "BenchmarkExecutor.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"

namespace vt_dstm {

int BenchmarkExecutor::calls = 1;
int BenchmarkExecutor::delay = 0;
long BenchmarkExecutor::timeout = 0;
bool BenchmarkExecutor::checkPoint = false;
int BenchmarkExecutor::objectsCount = -1;
int BenchmarkExecutor::transactions = 0 ;
int BenchmarkExecutor::readPercent = 0;
int BenchmarkExecutor::threads = 1;
unsigned long long BenchmarkExecutor::executionTime = 0;
bool BenchmarkExecutor::isInitiated = false;

HyflowBenchmark* BenchmarkExecutor::benchmark = NULL;
bool* BenchmarkExecutor::transactionType = NULL;
std::string** BenchmarkExecutor::argsArray = NULL;
std::string* BenchmarkExecutor::ids = NULL;

BenchmarkExecutor::BenchmarkExecutor() {
	// TODO Auto-generated constructor stub
}

BenchmarkExecutor::~BenchmarkExecutor() {
	// TODO Auto-generated destructor stub
}

unsigned long long BenchmarkExecutor::getTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + 0.001*tv.tv_usec;
}

void BenchmarkExecutor::writeResults() {
	unsigned long long trp =  transactions*(1000/executionTime);
	Logger::result("Throughput = %llu", trp);
}

std::string& BenchmarkExecutor::randomId() {
	int randomIndex = (rand() % objectsCount);
	return ids[randomIndex];
}

void BenchmarkExecutor::initExecutor(){
	if ( !isInitiated ) {
		benchmark = new BankBenchmark();
		objectsCount = atoi(ConfigFile::Value(OBJECTS).c_str());
		transactions = atoi(ConfigFile::Value(TRANSACTIONS).c_str());
		readPercent =  atoi(ConfigFile::Value(READS).c_str());
		isInitiated = true;
	}
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
		for (int j=0; j < argsCount; j++ ) {
			argsArray[i][j] = randomId();
		}
		if ( i < (transactions*readPercent/100))
			transactionType[i] = true;
		else
			transactionType[i] = false;
	}

	// Shuffle the transaction array
	for( int k = 0; k < transactions; k++ ) {
		int r = k + (rand() % (transactions - k));
		bool tmp = transactionType[k];
		transactionType[k] = transactionType[r];
		transactionType[r] = tmp;
	}
}

void BenchmarkExecutor::execute(){
	// Read all the configuration settings
	initExecutor();

	// Create objects and then make all nodes done populating objects
	createObjects();
	prepareArgs();
	NetworkManager::synchronizeCluster(2);
	sleep(2);
	int argsCount = benchmark->getOperandsCount();
	Logger::debug("BNCH_EXE :------------------------------>\n");
	unsigned long long startTime = getTime();
	for(int i=0; i < transactions; i++) {
		if (transactionType[i]) {
			benchmark->readOperation(argsArray[i], argsCount);
		} else {
			benchmark->writeOperation(argsArray[i], argsCount);
		}
	}
	executionTime = getTime() + 1 - startTime;
	Logger::debug("BNC_EXE: Execution time = %llu msec <----------------------\n", executionTime);

	writeResults();

	// Make sure all node finished transactions and then do sanity check
	NetworkManager::synchronizeCluster(3);
	benchmark->checkSanity();
	// Make sure sanity check is completed on all the nodes
	NetworkManager::synchronizeCluster(4);
	// DONE
}

} /* namespace vt_dstm */
