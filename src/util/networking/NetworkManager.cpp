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
#include "msgConnect/MSCtest.h"

namespace vt_dstm {

int NetworkManager::nodeId = -1;
int NetworkManager::nodeCount = 0;
int NetworkManager::machine = -1;
int NetworkManager::basePort = -1;

AbstractNetwork* NetworkManager::network = NULL;
bool NetworkManager::islocal = false;

void NetworkManager::NetworkInit() {
	if (strcmp(ConfigFile::Value(NETWORK).c_str(), MSG_CONNECT) == 0) {
		network = new MSCNetwork();
		HyflowMessage::registerMessageHandlers();
		network->initCluster();
	}
}

void NetworkManager::initNode() {
	if (nodeId == -1) {
		nodeId = atoi(ConfigFile::Value(NODE_ID).c_str());
		nodeCount = atoi(ConfigFile::Value(NODES).c_str());
		// Note Machine ID should be specified by user, not as in Hyflow java
		machine = atoi(ConfigFile::Value(MACHINES).c_str());
		basePort = atoi(ConfigFile::Value(BASE_PORT).c_str());
		if(atoi(ConfigFile::Value(MACHINES).c_str()) == 1)
			islocal = true;
	}
}

int NetworkManager::getNodeId(){
	return nodeId;
}

int NetworkManager::getNodeCount(){
	return nodeCount;
}

int NetworkManager::getMachine(){
	return machine;
}

int NetworkManager::getBasePort(){
	return basePort;
}

bool NetworkManager::allNodeJoined(){
	return network->allNodeJoined();
}

void NetworkManager::setClustered(){
	network->setClustered();
}

HyflowMessageFuture & NetworkManager::getMessageFuture(unsigned long long m_id, HyMessageType t) {
	return network->getMessageFuture(m_id, t);
}

void  NetworkManager::removeMessageFuture(unsigned long long m_id, HyMessageType t){
	network->removeMessageFuture(m_id, t);
}

void NetworkManager::registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &)) {
	network->registerHandler(msg_t, handlerFunc);
}

void NetworkManager::sendMessage(int nodeId, HyflowMessage msg) {
	msg.fromNode = NetworkManager::nodeId;
	msg.toNode = nodeId;

	network->sendMessage(nodeId, msg);
}

void NetworkManager::sendCallbackMessage(int nodeId, HyflowMessage msg, HyflowMessageFuture & fu) {
	network->sendCallbackMessage(nodeId, msg, fu);
}

bool NetworkManager::islocalMachine() {
	return islocal;
}

void NetworkManager::waitTillClustered(){
	network->waitTillClustered();
}


void NetworkManager::test() {
	std::cout << "\n---Testing Network---\n" << std::endl;
	if (strcmp(ConfigFile::Value(NETWORK).c_str(), MSG_CONNECT) == 0) {
		MSCtest::test();
	}
}

}
