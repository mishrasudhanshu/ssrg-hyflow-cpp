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
#include "../messages/MessageHandler.h"

namespace vt_dstm {

int NetworkManager::nodeId = -1;
int NetworkManager::nodeCount = 0;
int NetworkManager::machine = -1;
int NetworkManager::basePort = -1;
int  NetworkManager::threadCount = -1;

int NetworkManager::nodesInCluster=0;
int NetworkManager::syncVersion=0;
boost::condition NetworkManager::onCluster;
boost::mutex NetworkManager::clsMutex;

AbstractNetwork* NetworkManager::network = NULL;
bool NetworkManager::islocal = false;

std::string NetworkManager::Ips[] = {
		"10.1.1.20",
		"10.1.1.21",
		"10.1.1.22",
		"10.1.1.24",
		"10.1.1.25",
		"10.1.1.26",
		"10.1.1.27",
		"10.1.1.28",
		};

void NetworkManager::NetworkInit() {
	if (strcmp(ConfigFile::Value(NETWORK).c_str(), MSG_CONNECT) == 0) {
		MsgConnect::MCBaseInitialization();
		network = new MSCNetwork();
		HyflowMessage::registerMessageHandlers();
		synchronizeCluster(1);
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

void NetworkManager::synchronizeCluster(int rqNo) {
	SynchronizeMsg gJmsg(nodeId, false, rqNo);
	HyflowMessage hmsg;
	hmsg.setMsg(&gJmsg);
	hmsg.msg_t = MSG_GRP_SYNC;
	hmsg.isCallback = false;
	sendMessage(0,hmsg);
	waitTillSynchronized(rqNo);
}

bool NetworkManager::allNodeJoined(int rqNo){
	{
		boost::unique_lock<boost::mutex> lock(clsMutex);
		nodesInCluster++;
	}
	LOG_DEBUG("MSNC : Joining cluster in cluster %d in ReqNo %d\n", nodesInCluster, rqNo);
	return nodesInCluster == nodeCount;
}

void NetworkManager::replySynchronized(int rqNo){
	if (nodeId == 0) {
		SynchronizeMsg gJmsg(nodeId, true, rqNo);
		HyflowMessage hmsg;
		hmsg.setMsg(&gJmsg);
		hmsg.msg_t = MSG_GRP_SYNC;
		hmsg.isCallback = false;
		for (int i=0 ; i < nodeCount; i++)
			sendMessage(i,hmsg);
	}
}

void NetworkManager::notifyCluster(int rqNo){
	//Reset the nodes In cluster count
	nodesInCluster = 0;
	{
	     boost::unique_lock<boost::mutex> lock(clsMutex);
	     syncVersion = rqNo;
	 }
	 onCluster.notify_all();
	 LOG_DEBUG("MSNC : Notify all ReqNo %d\n", rqNo);
}

void NetworkManager::waitTillSynchronized(int rqNo){
	LOG_DEBUG("MSNC : Starting wait for syncVer %d and ReqNo %d\n", syncVersion, rqNo);
	boost::unique_lock<boost::mutex> lock(clsMutex);
	while ( syncVersion != rqNo) {
		onCluster.wait(lock);
	}
}

//void NetworkManager::registerMessageFuture(unsigned long long m_id, HyMessageType t, HyflowMessageFuture & fu) {
//	network->registerMessageFuture(m_id, t, fu);
//}
//
//HyflowMessageFuture & NetworkManager::getMessageFuture(unsigned long long m_id, HyMessageType t) {
//	return network->getMessageFuture(m_id, t);
//}
//
//void  NetworkManager::removeMessageFuture(unsigned long long m_id, HyMessageType t){
//	network->removeMessageFuture(m_id, t);
//}
//
//void NetworkManager::registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &)) {
//	network->registerHandler(msg_t, handlerFunc);
//}

void NetworkManager::sendMessage(int nodeId, HyflowMessage msg) {
	msg.fromNode = NetworkManager::nodeId;
	msg.toNode = nodeId;

	network->sendMessage(nodeId, msg);
}

void NetworkManager::sendCallbackMessage(int targetNodeId, HyflowMessage msg, HyflowMessageFuture & fu) {
	msg.fromNode = NetworkManager::nodeId;
	msg.toNode = targetNodeId;

	fu.setType(msg.msg_t);
	fu.setForObjectId(msg.getForObjectId());

	fu.createIdNRegisterFuture();
	msg.msg_id = fu.getId();
//	int threadId = BenchmarkExecutor::getThreadId();
//	msg.msg_id = getCurrentTime()*10000 + 100*targetNodeId + threadId;	// Max 19 Digits
//	fu.setId(msg.msg_id);
//	MessageHandler::registerMessageFuture(msg.msg_id, msg.msg_t, fu);

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
	//FIXME: Complete the id dependent implementation
	if (islocalMachine())
		return "127.0.0.1";
	int mac = NetworkManager::getMachine();
	return Ips[mac];
}

}
