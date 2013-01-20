/*
 * BenchmarkExecutor.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include <cstdlib>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "BenchmarkExecutor.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"
#include "../core/helper/RandomIdProvider.h"
#include "../util/concurrent/ThreadMeta.h"

namespace vt_dstm {

void HyflowMetaData::updateMetaData(HyflowMetaData & metadata, HyflowMetaDataType type) {
	switch (type) {
	case HYFLOW_METADATA_RUNTIME:
		txnRunTime += metadata.txnRunTime;
		break;
	case HYFLOW_METADATA_COMMITED_TXNS:
		committedTxns += metadata.committedTxns;
		break;
	case HYFLOW_METADATA_TRIES:
		txnTries.setValue(txnTries.getValue()+metadata.txnTries.getValue());
		break;
	case HYFLOW_METADATA_ABORTS:
		txnAborts.setValue(txnAborts.getValue()+metadata.txnAborts.getValue());
		break;
	case HYFLOW_METADATA_CHECKPOINT_RESUME:
		txnCheckpointResume.setValue(txnCheckpointResume.getValue()+metadata.txnCheckpointResume.getValue());
		break;
	case HYFLOW_METADATA_COMMITED_SUBTXNS:
		committedSubTxns += metadata.committedSubTxns;
		break;
	case HYFLOW_METADATA_COMMITED_SUBTXN_TIME:
		committedSubTxnTime += metadata.committedSubTxnTime;
		break;
	case HYFLOW_METADATA_ABORTED_SUBTXNS:
		abortedSubTxns += metadata.abortedSubTxns;
		break;
	case HYFLOW_METADATA_ABORTED_SUBTXN_TIME:
		abortedSubTxnTime += metadata.abortedSubTxnTime;
		break;
	case HYFLOW_METADATA_COMPENSATE_SUBTXNS:
		compensateSubTxns += metadata.compensateSubTxns;
		break;
	case HYFLOW_METADATA_COMPENSATE_SUBTXN_TIME:
		compensateSubTxnTime += metadata.compensateSubTxnTime;
		break;
	case HYFLOW_METADATA_BACKOFF_TIME:
		backOffTime += metadata.backOffTime;
		break;
	case HYFLOW_METADATA_ALL:
		txnRunTime += metadata.txnRunTime;
		committedTxns += metadata.committedTxns;
		txnTries.setValue(txnTries.getValue()+metadata.txnTries.getValue());
		txnAborts.setValue(txnAborts.getValue()+metadata.txnAborts.getValue());
		txnCheckpointResume.setValue(txnCheckpointResume.getValue()+metadata.txnCheckpointResume.getValue());
		committedSubTxns += metadata.committedSubTxns;
		committedSubTxnTime += metadata.committedSubTxnTime;
		abortedSubTxns += metadata.abortedSubTxns;
		abortedSubTxnTime += metadata.abortedSubTxnTime;
		compensateSubTxns += metadata.compensateSubTxns;
		compensateSubTxnTime += metadata.compensateSubTxnTime;
		backOffTime += metadata.backOffTime;
		break;
	default:
		Logger::fatal("HYMETA :Invalid HyflowMetaData type\n");
		break;
	}
}

void HyflowMetaData::increaseMetaData(HyflowMetaDataType type) {
	switch (type) {
	case HYFLOW_METADATA_COMMITED_TXNS:
		committedTxns++;
		break;
	case HYFLOW_METADATA_TRIES:
		txnTries.increaseValue();
		break;
	case HYFLOW_METADATA_ABORTS:
		txnAborts.increaseValue();
		LOG_DEBUG("HYMETA :---tries++-->%d\n", txnAborts.getValue());
		break;
	case HYFLOW_METADATA_CHECKPOINT_RESUME:
		txnCheckpointResume.increaseValue();
		LOG_DEBUG("HYMETA :---Checkpoint++-->%d\n", txnCheckpointResume.getValue());
		break;
	case HYFLOW_METADATA_COMMITED_SUBTXNS:
		committedSubTxns++;
		break;
	case HYFLOW_METADATA_ABORTED_SUBTXNS:
		abortedSubTxns++;
		break;
	case HYFLOW_METADATA_COMPENSATE_SUBTXNS:
		compensateSubTxns++;
		break;
	default:
		Logger::fatal("HYMETA :Invalid HyflowMetaData type\n");
		break;
	}
}

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
int BenchmarkExecutor::threadCount = 0;
int BenchmarkExecutor::transactionLength=0;
int BenchmarkExecutor::innerTxns=1;
int BenchmarkExecutor::itcpr=1;
int BenchmarkExecutor::objectNesting=2;
unsigned long BenchmarkExecutor::executionTime=0;
boost::mutex* BenchmarkExecutor::metaWriteMutex=NULL;

HyflowMetaData BenchmarkExecutor::benchNodeMetadata;
std::string BenchmarkExecutor::benchMarkName;

HyflowBenchmark* BenchmarkExecutor::benchmark = NULL;
bool* BenchmarkExecutor::transactionType = NULL;
std::string*** BenchmarkExecutor::threadArgsArray = NULL;
std::string* BenchmarkExecutor::ids = NULL;
HyflowMetaData* BenchmarkExecutor::benchMarkThreadMetadata = NULL;
pthread_t* BenchmarkExecutor::benchmarkThreads = NULL;

BenchmarkExecutor::BenchmarkExecutor() {}

BenchmarkExecutor::~BenchmarkExecutor() {}

unsigned long long BenchmarkExecutor::getTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}

void BenchmarkExecutor::submitThreadMetaData(HyflowMetaData& threadMetadata) {
	benchNodeMetadata.updateMetaData(threadMetadata, HYFLOW_METADATA_ALL);
}

void BenchmarkExecutor::writeResults() {
	double throughPut = (double(benchNodeMetadata.committedTxns*1000000))/(benchNodeMetadata.txnRunTime+1);
	LOG_DEBUG("BE :Throughput=%f, retryCount=%d, total tries=%d\n", throughPut, benchNodeMetadata.txnAborts.getValue(),
			benchNodeMetadata.txnTries.getValue());
	Logger::result("Throughput=%.2f\n", throughPut);
	Logger::result("Trnxs=%d\n", benchNodeMetadata.committedTxns);
	float abortRate = ((float)benchNodeMetadata.txnAborts.getValue()*100)/(benchNodeMetadata.txnTries.getValue());
	Logger::result("AbortRate=%.2f\n",abortRate);
	Logger::result("CheckpointResume=%d\n",benchNodeMetadata.txnCheckpointResume.getValue());
	Logger::result("CommittedSubTxns=%u\n", benchNodeMetadata.committedSubTxns);
	Logger::result("CommittedSubTxnTime=%llu\n", benchNodeMetadata.committedSubTxnTime/1000);	// MilliSecond
	Logger::result("AbortedSubTxns=%u\n", benchNodeMetadata.abortedSubTxns);
	Logger::result("AbortedSubTxnTime=%llu\n", benchNodeMetadata.abortedSubTxnTime/1000);
	Logger::result("CompensateSubTxns=%u\n", benchNodeMetadata.compensateSubTxns);
	Logger::result("CompensateSubTxnTime=%llu\n", benchNodeMetadata.compensateSubTxnTime/1000);
	Logger::result("BackoffTime=%llu\n", benchNodeMetadata.backOffTime);
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
		benchmarkThreads = new pthread_t[threadCount];
		sanity = (strcmp(ConfigFile::Value(SANITY).c_str(), TRUE) == 0)? true:false;
		transactionLength = atoi(ConfigFile::Value(TRANSACTIONS_LENGTH).c_str());
		executionTime = strtoul(ConfigFile::Value(EXECUTION_TIME).c_str(), NULL, 10);
		int it = atoi(ConfigFile::Value(INNER_TXNS).c_str());
		if (it>0) {
			innerTxns = it;
		}
		itcpr = atoi(ConfigFile::Value(ITCPR).c_str());
		objectNesting = atoi(ConfigFile::Value(OBJECT_NESTING).c_str());
		benchMarkThreadMetadata = new HyflowMetaData[threadCount];
		metaWriteMutex = new boost::mutex[threadCount];
		writeConfig();
	}
}

void BenchmarkExecutor::writeConfig() {
	Logger::result("----------%llu------------\n", getTime());
	Logger::result("Benchmark=%s\n", benchMarkName.c_str());
	Logger::result("Reads=%d%%\n", readPercent);
	Logger::result("Objects=%d\n", objectsCount);
	Logger::result("Threads=%d\n",threadCount);
#ifdef RELEASE
	Logger::result("RunMode=Release\n");
#else
	Logger::result("RunMode=Debug\n");
#endif
	Logger::result("ExecutionTime=%d\n",executionTime);
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

void BenchmarkExecutor::executeTransaction(int txnId, int id) {
	int argsCount = benchmark->getOperandsCount();
	std::string** argsArray = threadArgsArray[id];

	int pos = (txnId + id) % transactions;
	unsigned long long start = getTime();
	if (transactionType[pos]) {
		benchmark->readOperation(argsArray[pos], argsCount);
	} else {
		benchmark->writeOperation(argsArray[pos], argsCount);
	}
	unsigned long long txnTime = getTime() - start;	// Get value in us
	HyflowMetaData runTime;
	runTime.txnRunTime = txnTime;
	updateMetaData(runTime, HYFLOW_METADATA_RUNTIME);
	increaseMetaData(HYFLOW_METADATA_COMMITED_TXNS);
}

void* BenchmarkExecutor::execute(void* threadId){
	int id = *((int*)threadId);
	ThreadMeta::threadInit(id, TRANSACTIONAL_THREAD);

	LOG_DEBUG("BNCH_EXE %d:------------------------------>\n", id);
	for(int i=0; i < transactions; i++) {
		executeTransaction(i, id);
	}
	LOG_DEBUG("BNCH_EXE %d:<------------------------------\n", id);
	return NULL;
}

void BenchmarkExecutor::updateMetaData(HyflowMetaData data, HyflowMetaDataType type) {
	int id = ThreadMeta::getThreadId();
	boost::lock_guard<boost::mutex> metalock(metaWriteMutex[id]);
	benchMarkThreadMetadata[id].updateMetaData(data, type);
}

void BenchmarkExecutor::increaseMetaData(HyflowMetaDataType type) {
	int id = ThreadMeta::getThreadId();
	boost::lock_guard<boost::mutex> metalock(metaWriteMutex[id]);
	benchMarkThreadMetadata[id].increaseMetaData(type);
}

bool BenchmarkExecutor::executeThreads() {
	bool forcedExit=false;
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
		int* id = new int;
		*id = i;
		pthread_create(&benchmarkThreads[i], NULL, execute, id);
	}

	//Get lock on MetaData and print it
	if (executionTime > 0) {
		sleep(executionTime);
		for (int i=0; i < threadCount ; i++) {
			boost::lock_guard<boost::mutex> metalock(metaWriteMutex[i]);
			submitThreadMetaData(benchMarkThreadMetadata[i]);
		}
		writeResults();
	}

	for (int i=0; i < threadCount ; i++) {
		if (executionTime > 0) {
//			pthread_kill(benchmarkThreads[i], SIGINT);
		}else {
			pthread_join(benchmarkThreads[i], NULL);
			submitThreadMetaData(benchMarkThreadMetadata[i]);
		}
	}

	if (!executionTime) {
		writeResults();
		sleep(2);

		// Make sure all node finished transactions and then do sanity check
		NetworkManager::synchronizeCluster();
		if ( (NetworkManager::getNodeId() == 0) && (sanity) && !forcedExit ) {
			benchmark->checkSanity();
		}
		sleep(2);	// Require to stop out of order synchronize request
		// Make sure sanity check is completed on all the nodes
		NetworkManager::synchronizeCluster();
	}

	// DONE
	return !executionTime;
}
} /* namespace vt_dstm */
