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
	static int threadCount;
	static std::string nodeIp;

	static bool islocal;
//	static int nodesInCluster;
	static int syncVersion;
	static boost::condition onCluster;
	static boost::mutex clsMutex;
	static std::map<int, int> syncMap;
	static std::map<int, std::string> ipMap;
public:
	static AbstractNetwork *network;

	static std::map<int, std::string> & getIpMap() {
		return ipMap;
	}

	static int getThreadCount();
	static void NetworkInit();
	static void NetworkShutdown();
	static void sendMessage(int nodeId, HyflowMessage Message);
	static void sendCallbackMessage(int nodeId, HyflowMessage Message, HyflowMessageFuture & fu);
//	static void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &));

	static void initNode();
	static int getNodeId();
	static int getNodeCount();
	static int getMachine();
	static int getBasePort();
	static std::string getIp(int id);
	static bool islocalMachine();

	/*
	 * Synchronize the whole cluster with respect to node 0
	 */
	static void synchronizeCluster();
	static bool allNodeJoined(int rqNo);
	static void replySynchronized(int rqNo);
	static void waitTillSynchronized(int rqNo);
	static void notifyCluster(int rqNo);
	static void registerNode(int nodeId, std::string & ipAddress);
	static void registerCluster(std::map<int, std::string> & nodeMap);
	static std::string getNodeIP();

//	static void registerMessageFuture(unsigned long long m_id, HyMessageType t, HyflowMessageFuture & fu);
//	static HyflowMessageFuture & getMessageFuture(unsigned long long m_id, HyMessageType t);
//	static void removeMessageFuture(unsigned long long m_id, HyMessageType t);
	static void test();
};

}
#endif /* NETWORKMANAGER_H_ */
