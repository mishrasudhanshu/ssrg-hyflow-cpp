/*
 * AbstractNetwork.h
 *
 *  Created on: Aug 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTNETWORK_H_
#define ABSTRACTNETWORK_H_

#include <cstdio>
#include "../messages/HyflowMessage.h"

namespace vt_dstm {

class AbstractNetwork {

public:
	virtual ~AbstractNetwork() {
	}
	;
	/*
	 * Initiate the Network setup for given node
	 */
	virtual void NetworkInit() {
	}
	;
	/**
	 * Send a message to given node
	 */
	virtual void sendMessage(int nodeId, HyflowMessage Message) {
	}
	;

	/**
	 * Send a message and wait for response Message
	 */
	virtual HyflowMessage sendCallbackMessage(int nodeId,
			HyflowMessage Message) {
		HyflowMessage msg;
		return msg;
	}
	;

	/**
	 * Register Message Handler for given type of Message
	 */
	virtual void registerHandler(HyMessageType msg_t, void(HyflowMessage)) {
	}
	;
};

}
#endif /* ABSTRACTNETWORK_H_ */
