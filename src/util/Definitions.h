/*
 * Definitions.h
 *
 *  Created on: Aug 11, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_



namespace vt_dstm
{

//TODO: Move all definitions to array and enum
#define DIRECTORY_MANAGER "directoryManager"
#define HOME_DIRECTORY "HomeDirectory"
#define ARROR_DIRECTORY "ArrowDirectory"
#define RELAY_DIRECTORY "RelayDirectory"
#define DTL2_DIRECTORY	"DTL2Directory"
#define TRACKER_DIRECTORY "TrackerDirectory"
#define CONTROLFLOW_DIRECTORY "ControlFlowDirectory"

#define NETWORK "network"
#define MSG_CONNECT "MsgConnect"
#define ZERO_MQ "zeroMQ"
#define ZERO_MQ_TFR "zeroMQTFR"
#define ZERO_MQ_WFR "zeroMQWFR"

#define LOGGER "logger"
#define BASIC_LOGGER "Basic"
#define PANTHEIOS_LOGGER "Pantheios"

#define CONTEXT "context"
#define EMPTY "empty"
#define DTL "dtl"
#define DTL2 "dtl2"
#define UNDO_LOG "undoLog"
#define CTRL_UNDO_LOG "control_undoLog"
#define CTRL_WRITEBUGGER "control_writeBuffer"

#define CONTENTION_POLICY "contentionPolicy"
#define KARMA "Karma"
#define RANDOM "Random"
#define KINDERGARTEN "Kindergarten"
#define TIME_STAMP "Timestamp"
#define POLITE "Polite"
#define AGGRESSIVE "Aggressive"
#define PRIORITY "Priority"

#define REMOTE_CALLER "remoteCaller"

#define VERBOSE "verbose"
#define HY_DEBUG "debug"
#define SANITY "sanity"
#define DO_WARMUP "warmUp"
#define INSTRUMENT "instrument"
#define CHECKPOINT "checkPoint"

#define LINK_DELAY "linkDelay"
#define CALL_COST "callCost"
#define PARENT_IP "parentIP"
#define MY_IP "myIP"
#define BASE_PORT "basePort"
#define TERMINATE_IDLE "terminateIdle"
#define MACHINES "machines"
#define UNIT_TEST "unitTest"

#define NODES "nodes"
#define NODE_ID "nodeId"
#define OBJECTS "objects"
#define OBJECT_NESTING "objectNesting"
#define CALLS "calls"
#define READS "reads"
#define THREADS "threads"
#define TRANSACTIONS "transactions"
#define TRANSACTIONS_LENGTH "transactionLength"
#define TIMEOUT "timeout"

#define BENCHMARK "benchmark"
#define BANK "bank"
#define LIST "list"
#define SKIP_LIST "skipList"
#define BST "bst"
#define LOAN "loan"
#define HASH_TABLE "hashTable"
#define TPCC "tpcc"
#define VACATION "vacation"
#define TEST "test"

#define NESTING_MODEL "nestingModel"
#define NESTING_NO "no_nesting"
#define NESTING_FLAT "flat"
#define NESTING_CLOSED "closed"
#define NESTING_OPEN "open"
#define NESTING_CHECKPOINT "checkPointing"

#define INNER_TXNS "innerTxns"
#define ITCPR "ITCPR"
#define CLUSTER_TIMEOUT "clusterTimeout"
#define EXECUTION_TIME "executionTime"
#define BASE_BACKOFF_TIME "baseBackoffTime"
#define BACKOFF_ALL_NESTING "backoffAllNesting"

#define TRUE "true"
#define FALSE "false"

}
#endif /* DEFINITIONS_H_ */
