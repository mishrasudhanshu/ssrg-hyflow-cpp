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

namespace vt_dstm
{

class NetworkManager {
	static int nodeId;
public:
	static AbstractNetwork *network;

	static void NetworkInit();
	static void sendMessage(int nodeId, HyflowMessage Message);
	static HyflowMessage sendCallbackMessage(int nodeId, HyflowMessage Message);
	static void registerHandler(HyMessageType msg_t, void (HyflowMessage));

	static int32_t getNodeId();
	static void setNodeId();
};

}
#endif /* NETWORKMANAGER_H_ */
