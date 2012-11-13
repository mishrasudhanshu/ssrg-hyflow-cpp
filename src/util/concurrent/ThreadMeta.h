/*
 * ThreadId.h
 *
 *  Created on: Sep 19, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef THREADMETA_H_
#define THREADMETA_H_

#include <sched.h>
#include <boost/thread/tss.hpp>
#include "HyInteger.h"

namespace vt_dstm {

enum ThreadType {
	TRANSACTIONAL_THREAD=0,
	MAIN_THREAD,
	FORWARDER_THREAD,
	CATCHER_THREAD,
	WORKER_THREAD,
	DISPATCHER_THREAD
};

class ThreadMeta {
	static boost::thread_specific_ptr<HyInteger> threadId;
public:
	ThreadMeta(){};
	virtual ~ThreadMeta(){};

	static void threadInit(int id, ThreadType T_type);
	static void threadDeinit(ThreadType T_type);
	static void setThreadId(int id);
	static int getThreadId();
};

} /* namespace vt_dstm */

#endif /* THREADMETA_H_ */
