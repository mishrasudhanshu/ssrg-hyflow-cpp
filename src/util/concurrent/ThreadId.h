/*
 * ThreadId.h
 *
 *  Created on: Sep 19, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef THREADID_H_
#define THREADID_H_

#include <boost/thread/tss.hpp>
#include "HyInteger.h"

namespace vt_dstm {

class ThreadId {
	static boost::thread_specific_ptr<HyInteger> threadId;
public:
	ThreadId(){};
	virtual ~ThreadId(){};

	static void setThreadId(int id) {
		threadId.reset(new HyInteger(id));
	}
	static int getThreadId() {
		if (!threadId.get()) {
			threadId.reset(new HyInteger(0));
		}
		return threadId.get()->getValue();
	}
};

} /* namespace vt_dstm */

#endif /* THREADID_H_ */
