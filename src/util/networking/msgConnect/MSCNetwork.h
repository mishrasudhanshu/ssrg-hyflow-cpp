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
	static std::string Ips[];
	static int threadCount;

	static int nodeCount;
	static int nodesInCluster;
	static int syncVersion;
	static boost::condition onCluster;
	static boost::mutex clsMutex;

	static volatile bool hyflowShutdown;
	static boost::thread **dispatchThread;

	MsgConnect::MCMessenger* messenger;
	MsgConnect::MCQueue* queue;
	MsgConnect::MCSocketTransport* socket;

	static ConcurrentHashMap<HyMessageType, void (*)(HyflowMessage &)> handlerMap;
	static ConcurrentHashMap<unsigned long long, HyflowMessageFuture*> trackerCallbackMap;
	static ConcurrentHashMap<unsigned long long, HyflowMessageFuture*> objCallbackMap;
	static ConcurrentHashMap<unsigned long long, HyflowMessageFuture*> syncCallbackMap;
	static ConcurrentHashMap<unsigned long long, HyflowMessageFuture*> lockCallbackMap;
	static ConcurrentHashMap<unsigned long long, HyflowMessageFuture*> readValidCallbackMap;
	static ConcurrentHashMap<unsigned long long, HyflowMessageFuture*> registerCallbackMap;
	static bool isInit;

	void messageDispatcher();
	unsigned long long getCurrentTime();
public:
	MSCNetwork();
	virtual ~MSCNetwork();

	void setupSockets();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
	void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &));

	void registerMessageFuture(unsigned long long m_id, HyMessageType t, HyflowMessageFuture & fu);
	HyflowMessageFuture & getMessageFuture(unsigned long long m_id, HyMessageType t);
	void removeMessageFuture(unsigned long long m_id, HyMessageType t);
	/**
	 * Returns true if all the nodes joined
	 */
	bool allNodeJoined(int rqNo);
	void replySynchronized(int rqNo);
	void waitTillSynchronized(int rqNo);
	void notifyCluster(int rqNo);

	static void defaultHandler(void* UserData, void* Sender,
		MsgConnect::MCMessage& Message, bool& Handled);
	static void callbackHandler(unsigned int UserData, MsgConnect::MCMessage& Message);

	static void dispatcher(MsgConnect::MCMessenger *mc);

	static std::string getIp(int id);
	static int getBasePort();

};

}
#endif /* MSCNETWORK_H_ */
