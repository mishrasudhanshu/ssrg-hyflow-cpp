/*
 * MCNetwork.h
 *
 *  Created on: Aug 16, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef MSCNETWORK_H_
#define MSCNETWORK_H_

#include <map>
#include <boost/thread.hpp>

#include "../AbstractNetwork.h"
#include "../../concurrent/ConcurrentHashMap.h"
#include "MsgConnect/MC.h"
#include "MsgConnect/MCBase.h"
#include "MsgConnect/MCSock.h"
#include "MsgConnect/MCSocket.h"

namespace vt_dstm
{
/*
 * MSCNetwork class wraps the MsgConnect and provide the required interface
 * to plug in network module.
 */

class MSCNetwork: public AbstractNetwork {
	static int nodeId;
	static int basePort;
	static int threadCount;

	static volatile bool hyflowShutdown;
	static boost::thread **dispatchThread;

	MsgConnect::MCMessenger* messenger;
	MsgConnect::MCQueue* queue;
	MsgConnect::MCSocketTransport* socket;

	static bool isInit;

	static void defaultHandler(void* UserData, void* Sender,
		MsgConnect::MCMessage& Message, bool& Handled);
	static void callbackHandler(unsigned int UserData, MsgConnect::MCMessage& Message);
	static void dispatcher(MsgConnect::MCMessenger *mc, int dispatcherId);
public:
	MSCNetwork();
	virtual ~MSCNetwork();

	void networkInit();
	void networkShutdown();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
};

}
#endif /* MSCNETWORK_H_ */
