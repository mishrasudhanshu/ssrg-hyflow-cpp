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

void HyflowMetaData::updateMetaData(HyflowMetaData & metadata, HyflowMetaDataType type) {
	switch (type) {
	case HYFLOW_METADATA_THROUGHPUT:
		throughPut += metadata.throughPut;
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
		throughPut += metadata.throughPut;
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

HyflowMetaData BenchmarkExecutor::benchNodeMetadata;
std::string BenchmarkExecutor::benchMarkName;

HyflowBenchmark* BenchmarkExecutor::benchmark = NULL;
bool* BenchmarkExecutor::transactionType = NULL;
std::string*** BenchmarkExecutor::threadArgsArray = NULL;
std::string* BenchmarkExecutor::ids = NULL;
boost::thread_specific_ptr<HyflowMetaData> BenchmarkExecutor::benchMarkThreadMetadata;
boost::thread** BenchmarkExecutor::benchmarkThreads = NULL;
boost::mutex BenchmarkExecutor::execMutex;

BenchmarkExecutor::BenchmarkExecutor() {}

BenchmarkExecutor::~BenchmarkExecutor() {}

unsigned long long BenchmarkExecutor::getTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}

void BenchmarkExecutor::submitThreadMetaData(HyflowMetaData& threadMetadata) {
	boost::unique_lock<boost::mutex> metaDatalock(execMutex);
	benchNodeMetadata.updateMetaData(threadMetadata, HYFLOW_METADATA_ALL);
}

void BenchmarkExecutor::writeResults() {
	LOG_DEBUG("Throughput=%f, retryCount=%d, total tries=%d\n", benchNodeMetadata.throughPut, benchNodeMetadata.txnAborts.getValue(),
			benchNodeMetadata.txnTries.getValue());
	Logger::result("Throughput=%.2f\n", benchNodeMetadata.throughPut);
	float abortRate = ((float)benchNodeMetadata.txnAborts.getValue()*100)/(benchNodeMetadata.txnTries.getValue()*threadCount);
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
	benchMarkThreadMetadata->throughPut = (double(transactions*1000000))/executionTime;

	if (benchMarkThreadMetadata.get()) {
		submitThreadMetaData(*(benchMarkThreadMetadata.get()));
		LOG_DEBUG("BNC_EXE %d: ThroughPut = %0.3f trxns/sec <----------------------\n", id, benchMarkThreadMetadata->throughPut);
	}
}

void BenchmarkExecutor::updateMetaData(HyflowMetaData data, HyflowMetaDataType type) {
	if (!benchMarkThreadMetadata.get()) {
		benchMarkThreadMetadata.reset(new HyflowMetaData());
	}
	benchMarkThreadMetadata->updateMetaData(data, type);
}

void BenchmarkExecutor::increaseMetaData(HyflowMetaDataType type) {
	if (!benchMarkThreadMetadata.get()) {
		benchMarkThreadMetadata.reset(new HyflowMetaData());
	}
	benchMarkThreadMetadata->increaseMetaData(type);
}

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
