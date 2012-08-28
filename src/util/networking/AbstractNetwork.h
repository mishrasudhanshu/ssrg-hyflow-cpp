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
	virtual void setupSockets() {
	}
	;
	/**
	 * Send a message to given node
	 */
	virtual void sendMessage(int nodeId, HyflowMessage & Message) {
	}
	;

	/**
	 * Send a message and wait for response Message
	 */
	virtual HyflowMessage & sendCallbackMessage(int nodeId,
			HyflowMessage & Message)=0;

	/**
	 * Register Message Handler for given type of Message
	 */
	virtual void registerHandler(HyMessageType msg_t, void(*handlerFunc)(HyflowMessage &)) =0;

	/**
	 * Initiate cluster function if network library inherently does not support it.
	 */
	virtual void initCluster(){};

	/**
	 * Get Message by its id, so that callback message may be updated
	 */
	virtual HyflowMessage & getMessageById(unsigned long long m_id, HyMessageType t)=0;
};

}
#endif /* ABSTRACTNETWORK_H_ */
