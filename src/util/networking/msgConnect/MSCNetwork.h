/*
 * MCNetwork.h
 *
 *  Created on: Aug 16, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef MSCNETWORK_H_
#define MSCNETWORK_H_

#include <map>

#include "../AbstractNetwork.h"
#include "MC.h"
#include "MCBase.h"
#include "MCSock.h"
#include "MCSocket.h"

namespace vt_dstm
{
/*
 * MSCNetwork class wraps the MsgConnect and provide the required interface
 * to plug in network module.
 */

class MSCNetwork: public AbstractNetwork {
	static MSCNetwork *instance;

	MsgConnect::MCMessenger* messenger;
	MsgConnect::MCQueue* queue;
	MsgConnect::MCSocketTransport* socket;
	MsgConnect::MCMessageHandlers* handlers;

	std::map<HyMessageType, void (*)(HyflowMessage)> handlerMap;

public:
	MSCNetwork();
	virtual ~MSCNetwork();

	void NetworkInit();
	void sendMessage(int nodeId, HyflowMessage Message);
	HyflowMessage sendCallbackMessage(int nodeId, HyflowMessage Message);
	void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage));
	static void defaultHandler(void* UserData, void* Sender,
		MsgConnect::MCMessage& Message, bool& Handled);
};

}
#endif /* MSCNETWORK_H_ */
