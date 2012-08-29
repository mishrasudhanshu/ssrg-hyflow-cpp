/*
 * ClusterManager.h
 *
 *  Created on: Aug 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <stdint.h>

#include "AbstractNetwork.h"
#include "../messages/HyflowMessage.h"
#include "../messages/HyflowMessageFuture.h"

namespace vt_dstm
{

class NetworkManager {
	static int nodeId;
	static int nodeCount;
	static int machine;
	static int basePort;
	// FIXME: Make following conditional variables
	static volatile int nodeJoined;
	static volatile bool isCluster;
	static bool islocal;
public:
	static AbstractNetwork *network;

	static void NetworkInit();
	static void sendMessage(int nodeId, HyflowMessage Message);
	static void sendCallbackMessage(int nodeId, HyflowMessage Message, HyflowMessageFuture & fu);
	static void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &));

	static void initNode();
	static int getNodeId();
	static int getNodeCount();
	static int getMachine();
	static int getBasePort();
	static void atomicIncreaseNodeJoined();
	static void setClustered();
	static bool islocal();

	static HyflowMessageFuture & getMessageFuture(unsigned long long m_id, HyMessageType t);
	static void removeMessageFuture(unsigned long long m_id, HyMessageType t);
	static void test();
};

}
#endif /* NETWORKMANAGER_H_ */
