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
	virtual ~AbstractNetwork() {}
	;
	/*
	 * Initiate the Network setup for given node
	 */
	virtual void networkInit() =0;
	/*
	 * Shutdown the Network
	 */
	virtual void networkShutdown() =0;
	/*
	 * Initiate thread specific networking settings
	 */
	virtual void threadNetworkInit(){};
	virtual void threadNetworkShutdown(){};
	/**
	 * Send a message to given node
	 */
	virtual void sendMessage(int nodeId, HyflowMessage & Message) =0;

	/**
	 * Send a message and wait for response Message
	 */
	virtual void sendCallbackMessage(int nodeId,
			HyflowMessage & Message, HyflowMessageFuture & fu)=0;

	virtual void registerNode(int nodeId) {}
	virtual void registerCluster(std::map<int, std::string> & nodeMap) {}
};

}
#endif /* ABSTRACTNETWORK_H_ */
