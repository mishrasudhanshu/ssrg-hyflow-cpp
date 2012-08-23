/*
 * ClusterManager.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: mishras[at]vt.edu
 */
#include <string>
#include <cstdlib>

#include "NetworkManager.h"
#include "../parser/ConfigFile.h"
#include "../Definitions.h"
#include "msgConnect/MSCNetwork.h"

namespace vt_dstm {

int32_t NetworkManager::nodeId = -1;
AbstractNetwork* NetworkManager::network = NULL;

void NetworkManager::NetworkInit() {
	if (strcmp(ConfigFile::Value(NETWORK).c_str(), MSG_CONNECT) == 0) {
		network = new MSCNetwork();
		network->NetworkInit();
	}

	// Register Default handlers
	// Register Directory handlers

	// Initiate the Group

	// Initiate cluster

	//FIXME: All below in Network library dependent code
	if (nodeId == 0) {
		// Wait for all nodes to join
		// add sockets for each node
		// send group-list to all nodes
		// wait for cluster creation from all nodes
		// signal go head
	} else {
		// Sleep for 5 sec to make sure node initiated its network
		// spend you group joining news
		// Wait for group list
		// add sockets for each node
		// Send cluster creation news to zero
		// wait for go ahead signal
	}
}

int32_t NetworkManager::getNodeId() {
	return nodeId;
}

void NetworkManager::setNodeId() {
	if (nodeId == -1) {
		nodeId = atoi(ConfigFile::Value(NODE_ID).c_str());
	}
}

}
