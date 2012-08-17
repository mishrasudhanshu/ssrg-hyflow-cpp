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
#include "AbstractMessage.h"

namespace vt_dstm
{

class NetworkManager {
	static int nodeId;
public:
	static AbstractNetwork *network;

	static void NetworkInit();
	static void sendMessage(int nodeId, AbstractMessage Message);
	static AbstractMessage sendCallbackMessage(int nodeId, AbstractMessage Message);
	static void registerHandler(MessageType msg_t, void (AbstractMessage));

	static int32_t getNodeId();
	static void setNodeId();
};

}
#endif /* NETWORKMANAGER_H_ */
