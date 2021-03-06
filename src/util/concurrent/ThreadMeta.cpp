/*
 * ThreadId.cpp
 *
 *  Created on: Sep 19, 2012
 *      Author: mishras[at]vt.edu
 */
#include <sched.h>
#include "ThreadMeta.h"
#include "../networking/NetworkManager.h"

namespace vt_dstm{
	boost::thread_specific_ptr<HyInteger> ThreadMeta::threadId;

	void ThreadMeta::setThreadId(int id) {
		threadId.reset(new HyInteger(id));
	}

	int ThreadMeta::getThreadId() {
		if (!threadId.get()) {
			threadId.reset(new HyInteger(0));
		}
		return threadId.get()->getValue();
	}

	void ThreadMeta::threadInit(int id, ThreadType T_type) {
//		int coreId=0;
//		int nodeId = NetworkManager::getNodeId();
//		int threadCount = NetworkManager::getThreadCount();
		if (T_type == TRANSACTIONAL_THREAD) {
//			coreId = nodeId*threadCount + id;
			setThreadId(id);
			NetworkManager::threadNetworkInit();
		}else if (T_type == MAIN_THREAD ) {
			setThreadId(id);
		}else if (T_type == DISPATCHER_THREAD ){
//			coreId = nodeId*threadCount + id;
			// For dispatch thread default Id should be zero
			id = 0;
			setThreadId(id);
		}else if (T_type == WORKER_THREAD) {
			id = NetworkManager::getThreadCount() + id + 1;
			setThreadId(id);
		}else {
			id = 0;
			setThreadId(id);
		}

		// Leave two additional cores for Msgconnect server listener and worker thread
		// Per node core used = 4*threadCount
		// Set the thread affinity
//		cpu_set_t s;
//		CPU_ZERO(&s);
//		CPU_SET(coreId, &s);
//		if (sched_setaffinity(0, sizeof(cpu_set_t), &s) < 0){
//			perror("Set affinity transactional thread");
//		}


	   // Causing system to hang
//	   struct sched_param sp;
//	   int policy;
//
//	   if((policy = sched_getscheduler(0) == -1)) {
//		  perror("Failure in retrieving schedulers");
//	   }
//
//	   if(policy == SCHED_OTHER) {
//		  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
//		  if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {
//			  perror("Failure in setting schedulers");
//		  }
//	   }
	}

	void ThreadMeta::threadDeinit(ThreadType T_type) {
		if (T_type!=DISPATCHER_THREAD ) {
			NetworkManager::threadNetworkShutdown();
		}
	}
}


