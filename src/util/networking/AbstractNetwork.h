/*
 * AbstractNetwork.h
 *
 *  Created on: Aug 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTNETWORK_H_
#define ABSTRACTNETWORK_H_

#include "AbstractMessage.h"

namespace vt_dstm
{

class AbstractNetwork {

public:
	virtual ~AbstractNetwork(){};
	/*
	 * Initiate the Network setup for given node
	 */
	virtual void NetworkInit(){};
	/**
	 * Send a message to given node
	 */
	virtual void sendMessage(int nodeId, AbstractMessage Message){};

	/**
	 * Send a message and wait for response Message
	 */
	virtual AbstractMessage sendCallbackMessage(int nodeId, AbstractMessage Message){};

	/**
	 * Register Message Handler for given type of Message
	 */
	virtual void registerHandler(MessageType msg_t, void (AbstractMessage)){};
};

}
#endif /* ABSTRACTNETWORK_H_ */
