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
#include "../messages/HyflowMessageFuture.h"

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
	virtual void sendMessage(int nodeId, HyflowMessage & Message) =0;

	/**
	 * Send a message and wait for response Message
	 */
	virtual void sendCallbackMessage(int nodeId,
			HyflowMessage & Message, HyflowMessageFuture & fu)=0;

	/**
	 * Register Message Handler for given type of Message
	 */
	virtual void registerHandler(HyMessageType msg_t, void(*handlerFunc)(HyflowMessage &)) =0;
	/**
	 * Register Message Future
	 */
	virtual void registerMessageFuture(unsigned long long m_id, HyMessageType t, HyflowMessageFuture & fu) = 0;
	/**
	 * Get Message by its id, so that callback message may be updated
	 */
	virtual HyflowMessageFuture & getMessageFuture(unsigned long long m_id, HyMessageType t)=0;

	virtual void removeMessageFuture(unsigned long long m_id, HyMessageType t)=0;
	/*
	 * Adds the calling message to nodeCount and returns
	 * true if all the nodes have joined the cluster
	 */
	virtual bool allNodeJoined(int rqNo)=0;
	/*
	 * Notify the network that all the required nodes have joined the cluster
	 */
	virtual void setSynchronized(int rqNo)=0;
	/*
	 * Call will return when all the nodes have responded to node 0
	 */
	virtual void waitTillSynchronized(int rqNo)=0;
};

}
#endif /* ABSTRACTNETWORK_H_ */
