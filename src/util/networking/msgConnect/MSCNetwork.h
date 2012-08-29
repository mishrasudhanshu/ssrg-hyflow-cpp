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
	static int nodeCount;
	static int nodeId;
	static int basePort;
	static std::string Ips[];
	static std::string *nodeIps;
	MsgConnect::MCMessenger* messenger;
	MsgConnect::MCQueue* queue;
	MsgConnect::MCSocketTransport* socket;

	static std::map<HyMessageType, void (*)(HyflowMessage &)> handlerMap;
	static std::map<unsigned long long, HyflowMessageFuture*> trackerCallbackMap;
	static std::map<unsigned long long, HyflowMessageFuture*> objCallbackMap;

	void messageDispatcher();
	unsigned long long getCurrentTime();
public:
	MSCNetwork();
	virtual ~MSCNetwork();

	void setupSockets();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
	void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &));
	void initCluster();
	HyflowMessageFuture & getMessageFuture(unsigned long long m_id, HyMessageType t);
	void removeMessageFuture(unsigned long long m_id, HyMessageType t);

	static void defaultHandler(void* UserData, void* Sender,
		MsgConnect::MCMessage& Message, bool& Handled);
	static void callbackHandler(unsigned int UserData, MsgConnect::MCMessage& Message);
	static std::string getIp(int id);
	static int getBasePort();
};

}
#endif /* MSCNETWORK_H_ */
