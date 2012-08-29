/*
 * HyflowMessageFuture.h
 * A class for Hyflow providing asynchronous wait
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWMESSAGEFUTURE_H_
#define HYFLOWMESSAGEFUTURE_H_

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include "HyflowMessage.h"

namespace vt_dstm {

class HyflowMessageFuture {
	bool isReceived;
	unsigned long long msg_id;
	HyMessageType msg_t;
	boost::condition_variable onReceive;
	boost::mutex msgMutex;

public:
	HyflowMessageFuture();
	virtual ~HyflowMessageFuture();
	void waitOnFuture();
	bool isComplete();
	void notifyMessage();
	void setId(unsigned long long id);
	unsigned long long getId();
	void setType(HyMessageType t);
	HyMessageType getType();
};

} /* namespace vt_dstm */

#endif /* HYFLOWMESSAGEFUTURE_H_ */
