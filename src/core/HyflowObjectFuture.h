/*
 * HyflowObjectFuture.h
 * A wrapper class for Hyflow object which allows the user to make object
 * request asynchronously.
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWOBJECTFUTURE_H_
#define HYFLOWOBJECTFUTURE_H_

#include "../util/messages/HyflowMessageFuture.h"

namespace vt_dstm {

class HyflowObjectFuture {
	HyflowMessageFuture messageFuture;
	std::string objectId;
	bool isRead;
	unsigned long long txnId;
public:
	HyflowObjectFuture(std::string id, bool rw, unsigned long long txnId);
	virtual ~HyflowObjectFuture();
	HyflowObject & waitOnObject();
	bool isObjectAvailable();
	/**
	 * Internal Use only
	 */
	HyflowMessageFuture & getMessageFuture() {return messageFuture;}
};

} /* namespace vt_dstm */

#endif /* HYFLOWOBJECTFUTURE_H_ */
