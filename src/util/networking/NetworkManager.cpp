/*
 * ClusterManager.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: mishras[at]vt.edu
 */
#include <string>
#include <cstdlib>
#include <string.h>
#include <time.h>

#include "NetworkManager.h"
#include "../parser/ConfigFile.h"
#include "../logging/Logger.h"
#include "../Definitions.h"
#include "../messages/HyflowMessage.h"
#include "../messages/HyflowMessageFuture.h"
#include "../messages/types/SynchronizeMsg.h"
#include "msgConnect/MSCNetwork.h"
#include "msgConnect/MSCtest.h"
#include "zeroMQ/ZMQNetwork.h"
#include "zeroMQ/ZMQNetworkAsync.h"
#include "../messages/MessageHandler.h"
#include "IPAddressProvider.h"

namespace vt_dstm {

int NetworkManager::nodeId = -1;
int NetworkManager::nodeCount = 0;
int NetworkManager::machine = -1;
int NetworkManager::basePort = -1;
int  NetworkManager::threadCount = -1;

std::string NetworkManager::nodeIp;
int NetworkManager::syncVersion=0;
boost::condition NetworkManager::onCluster;
boost::mutex NetworkManager::clsMutex;

AbstractNetwork* NetworkManager::network = NULL;
bool NetworkManager::islocal = false;
std::map<int, int> NetworkManager::syncMap ;
std::map<int, std::string> NetworkManager::ipMap ;

void NetworkManager::NetworkInit() {
	nodeIp = IPAddressProvider::getIPv4Address();
	if (strcmp(ConfigFile::Value(NETWORK).c_str(), MSG_CONNECT) == 0) {
		MsgConnect::MCBaseInitialization();
		network = new MSCNetwork();
		HyflowMessage::registerMessageHandlers();
		synchronizeCluster();
	}else if (strcmp(ConfigFile::Value(NETWORK).c_str(), ZERO_MQ) == 0) {
//		network = new ZMQNetwork();
		network = new ZMQNetworkAsync();
		HyflowMessage::registerMessageHandlers();
		synchronizeCluster();
		network->networkInit();
	}
	sleep(2);
}

void NetworkManager::NetworkShutdown() {
	LOG_DEBUG("Shutting Down the Network!!!\n");
	// Allow network library to do clean up
	network->networkShutdown();
	AbstractNetwork *saveNetwork = network;
	network = NULL;
	delete saveNetwork;
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
		threadCount = atoi(ConfigFile::Value(THREADS).c_str());
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

void NetworkManager::synchronizeCluster() {
	int rqNo = syncVersion + 1;
	SynchronizeMsg gJmsg(nodeId, false, rqNo);
	HyflowMessage hmsg;
	hmsg.setMsg(&gJmsg);
	hmsg.msg_t = MSG_GRP_SYNC;
	hmsg.isCallback = false;
	sendMessage(0,hmsg);
	waitTillSynchronized(rqNo);
}

bool NetworkManager::allNodeJoined(int rqNo){
	int currentNodes = 0;
	{
		boost::unique_lock<boost::mutex> lock(clsMutex);
		std::map<int, int>::iterator i = syncMap.find(rqNo);
		if ( i == syncMap.end()) {
			syncMap[rqNo] = 1;
			currentNodes = 1;
		}else {
			currentNodes = i->second;
			syncMap[rqNo] = ++currentNodes;
		}
	}
	LOG_DEBUG("NETM : Joining cluster in cluster %d in ReqNo %d\n", currentNodes, rqNo);
	return nodeCount == currentNodes;
}

void NetworkManager::replySynchronized(int rqNo){
	if (nodeId == 0) {
		SynchronizeMsg gJmsg(nodeId, true, rqNo);
		HyflowMessage hmsg;
		hmsg.setMsg(&gJmsg);
		hmsg.msg_t = MSG_GRP_SYNC;
		hmsg.isCallback = false;
		for (int i=1 ; i < nodeCount; i++) {
			sendMessage(i,hmsg);
		}
		// For node zero locally wake up the thread
		notifyCluster(rqNo);
		LOG_DEBUG("NETM : Everyone Joined Awaking all\n");
	}
}

void NetworkManager::notifyCluster(int rqNo){
	{
	     boost::unique_lock<boost::mutex> lock(clsMutex);
	     syncMap.erase(rqNo);
	     syncVersion = rqNo;
	 }
	 onCluster.notify_all();
	 LOG_DEBUG("NETM : Awaking node - notify all waiting on ReqNo %d\n", rqNo);
}

void NetworkManager::waitTillSynchronized(int rqNo){
	LOG_DEBUG("NETM : Starting wait for syncVer %d and ReqNo %d\n", syncVersion, rqNo);
	boost::unique_lock<boost::mutex> lock(clsMutex);
	while ( syncVersion < rqNo) {
		onCluster.wait(lock);
	}
}

void NetworkManager::sendMessage(int nodeId, HyflowMessage & msg) {
	msg.fromNode = NetworkManager::nodeId;
	msg.toNode = nodeId;
	msg.isCallback = false;

	network->sendMessage(nodeId, msg);
}

void NetworkManager::sendCallbackMessage(int targetNodeId, HyflowMessage & msg, HyflowMessageFuture & fu) {
	msg.fromNode = NetworkManager::nodeId;
	msg.toNode = targetNodeId;
	msg.isCallback = true;

	fu.setType(msg.msg_t);
	fu.setForObjectId(msg.getForObjectId());

	fu.createIdNRegisterFuture();
	msg.msg_id = fu.getId();

	network->sendCallbackMessage(targetNodeId, msg, fu);
}

bool NetworkManager::islocalMachine() {
	return islocal;
}

int NetworkManager::getThreadCount(){
	return threadCount;
}

void NetworkManager::test() {
	std::cout << "\n---Testing Network---\n" << std::endl;
	if (strcmp(ConfigFile::Value(NETWORK).c_str(), MSG_CONNECT) == 0) {
		if (NetworkManager::getNodeCount() == 1)
			MSCtest::testbase();
		else {
			MSCtest::test();
		}
	}
}

std::string NetworkManager::getIp(int id){
	if (islocalMachine())
		return "127.0.0.1";

	if (id == 0) {
		return ConfigFile::Value(PARENT_IP);
	}
	if (id == nodeId) {
		return nodeIp;
	}
	std::map<int,std::string>::iterator i= ipMap.find(id);
	if ( i == ipMap.end()) {
		Logger::fatal("Unable to get IP for node %d\n",id);
		throw "Unable to get IP\n";
	}else {
		return ipMap.at(id);
	}
}

std::string NetworkManager::getNodeIP() {
	if (islocalMachine()) {
		return "127.0.0.1";
	}
	return nodeIp;
}

void NetworkManager::registerNode(int nodeId) {
	network->registerNode(nodeId);
}

void NetworkManager::registerNode(int nodeId, std::string & ipAddress) {
    boost::unique_lock<boost::mutex> lock(clsMutex);
	ipMap[nodeId] = ipAddress;
	network->registerNode(nodeId);
}

void NetworkManager::registerCluster(std::map<int, std::string> & nodeMap) {
	LOG_DEBUG("Registering the Node Map\n");
    boost::unique_lock<boost::mutex> lock(clsMutex);
	ipMap.insert(nodeMap.begin(), nodeMap.end());
}

}
