/*
 * HyflowMessageFuture.h
 * A class for Hyflow providing asynchronous wait
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWMESSAGEFUTURE_H_
#define HYFLOWMESSAGEFUTURE_H_

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include "HyflowMessage.h"

namespace vt_dstm {

class HyflowMessageFuture {
	bool isReceived;
	unsigned long long msg_id;
	HyMessageType msg_t;
	boost::condition onReceive;
	boost::mutex msgMutex;
	
	unsigned long long txnId;
	/*
	 * Following fields are used to return response of future request
	 */
	bool boolResponse;
	int intResponse;
	std::string stringResponse;
	HyflowObject* dataResponse;

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

	bool isBoolResponse() const;
	void setBoolResponse(bool boolResponse);
	HyflowObject* getDataResponse() const;
	void setDataResponse(HyflowObject* dataResponse);
	int getIntResponse() const;
	void setIntResponse(int intResponse);
	std::string getStringResponse() const;
	void setStringResponse(std::string stringResponse);
	unsigned long long getTxnId() const;
	void setTxnId(unsigned long long txnId);
};

} /* namespace vt_dstm */

#endif /* HYFLOWMESSAGEFUTURE_H_ */
