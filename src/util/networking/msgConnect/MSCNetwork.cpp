/*
 * MCNetwork.cpp
 *
 *  Created on: Aug 16, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "MSCNetwork.h"
#include "../NetworkManager.h"
//#include "MCtest.cpp"

namespace vt_dstm
{
int MSCNetwork::nodeCount = 0;
int MSCNetwork::nodeId = -1;
MSCNetwork* MSCNetwork::instance = NULL;
std::string* MSCNetwork::nodeIps = NULL;
std::string MSCNetwork::Ips[] = {
		"10.1.1.20",
		"10.1.1.21",
		"10.1.1.22",
		"10.1.1.24",
		"10.1.1.25",
		"10.1.1.26",
		"10.1.1.27",
		"10.1.1.28",
		};
int MSCNetwork::basePort = -1;

std::map<HyMessageType, void (*)(HyflowMessage &)> MSCNetwork::handlerMap;
std::map<unsigned long long, HyflowMessage*> MSCNetwork::trackerCallbackMap;
std::map<unsigned long long, HyflowMessage*> MSCNetwork::objCallbackMap;

MSCNetwork::MSCNetwork() {
	if (!instance) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		nodeIps = new std::string[nodeCount];

		MsgConnect::MCBaseInitialization();
		instance = new MSCNetwork();
		instance->messenger = new MsgConnect::MCMessenger();
		instance->queue = new MsgConnect::MCQueue[nodeCount];
		instance->socket = new MsgConnect::MCSocketTransport[nodeCount];
		setupSockets();
		//FIXME: Add a background thread to dispatch the messages
	}
}

MSCNetwork::~MSCNetwork() {
	delete instance->messenger;
	delete[] instance->queue;
	delete[] instance->socket;
}

void MSCNetwork::setupSockets(){
	for(int i=0 ; i<nodeCount ; i++){
		if ( i != nodeId ) {
			nodeIps[i] = getIp(i);
			char *ip = (char*)getIp(i).c_str();
			unsigned int port = getBasePort() + nodeId;
			std::string q = nodeId + "-queue";
			char *queue = (char*) q.c_str();

			instance->messenger->setMaxTimeout(ULONG_MAX);
			instance->socket->setActive(false);
			instance->socket->setAttemptsToConnect(1);
			instance->socket->setFailOnInactive(true);
			instance->socket->setMaxTimeout(900000l);

			instance->socket->setMessengerAddress(ip);
			instance->socket->setMessengerPort(port);
			instance->socket->setTransportMode(stmP2P);
			instance->socket->setMessenger(messenger);
			instance->socket->setActive(true);

			instance->queue->setQueueName(queue);
			instance->queue->setMessenger(messenger);

			MsgConnect::MCMessageHandlers *mhls = instance->queue->getHandlers();
			MsgConnect::MCMessageHandler* handler = mhls->Add();
			handler->setMsgCodeLow(1);
			handler->setMsgCodeHigh(1);
			handler->setOnMessage(MSCNetwork::defaultHandler);
			handler->setEnabled(true);
		}
	}
}

void MSCNetwork::sendMessage(int nodeId, HyflowMessage & message){
	std::stringstream destStr;
	destStr << "Socket:" << getIp(nodeId) << ":" << getBasePort()+nodeId << "|" << nodeId  <<"-queue";
	std::string destination = destStr.str();

	// Serialize the Message
	std::ostringstream net_stream;
	boost::archive::text_oarchive archive(net_stream);
	archive << message;
	std::string msg = net_stream.str();

	MsgConnect::MCMessage mcmsg;
	mcmsg.MsgCode = 1;
	mcmsg.Param1 = 0;
	mcmsg.Param2 = 0;
	mcmsg.DataType = MsgConnect::bdtVar;

	mcmsg.Data = (void *) msg.c_str();
	mcmsg.DataSize = msg.size();

	messenger->PostMessage(destination.c_str(),&mcmsg,NULL);
}

HyflowMessage & MSCNetwork::sendCallbackMessage(int nodeId, HyflowMessage & message){
	HyflowMessage *msg = new HyflowMessage() ;
	msg->isCallback = true;

	message.msg_id = getCurrentTime()*1000 + nodeId;
	msg->msg_id = message.msg_id;

	msg->fromNode = message.toNode;
	msg->toNode = message.fromNode;

	objCallbackMap[msg->msg_id] = msg;

	std::stringstream destStr;
	destStr << "Socket:" << getIp(nodeId) << ":" << getBasePort()+nodeId << "|" << nodeId  <<"-queue";
	std::string destination = destStr.str();

	// Serialize the message
	std::ostringstream net_stream;
	boost::archive::text_oarchive archive(net_stream);
	archive << message;
	std::string msgData = net_stream.str();

	MsgConnect::MCMessage mcmsg;
	mcmsg.MsgCode = 1;
	mcmsg.Param1 = 0;
	mcmsg.Param2 = 0;
	mcmsg.DataType = MsgConnect::bdtVar;

	mcmsg.Data = (void *) msgData.c_str();
	mcmsg.DataSize = msgData.size();

	messenger->SendMessageCallback(destination.c_str(), &mcmsg, &callbackHandler,0,NULL);

	return *msg;
}

void MSCNetwork::registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &)){
	handlerMap[msg_t] = handlerFunc;
}

void MSCNetwork::defaultHandler(void* UserData, void* Sender,
	MsgConnect::MCMessage& msg, bool& Handled) {
	if(msg.Data && (msg.DataSize > 0)) {
		std::string data((char*)msg.Data, msg.DataSize);
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		std::map<HyMessageType, void (*)(HyflowMessage &)>::const_iterator ci = handlerMap.find(req.msg_t);
		if (ci == handlerMap.end())
			throw "Invalid type message";
		ci->second(req);
		Handled = true;
	}
}

void MSCNetwork::callbackHandler(unsigned int UserData, MsgConnect::MCMessage& msg){
	if(msg.Data && (msg.DataSize > 0)) {
		std::string data((char*)msg.Data, msg.DataSize);
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		std::map<HyMessageType, void (*)(HyflowMessage &)>::const_iterator ci = handlerMap.find(req.msg_t);
		if (ci == handlerMap.end())
			throw "Invalid type message";
		ci->second(req);
	}
}
std::string MSCNetwork::getIp(int id){
	//FIXME: Complete the id dependent implementation
	int mac = NetworkManager::getMachine();
	return Ips[mac];
}

void MSCNetwork::initCluster()	{
	// Initiate cluster

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

void MSCNetwork::messageDispatcher(){
	while (1)
		messenger->DispatchMessages();
}

int MSCNetwork::getBasePort(){
	return NetworkManager::getBasePort();
}

unsigned long long MSCNetwork::getCurrentTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}

HyflowMessage & MSCNetwork::getMessageById(unsigned long long m_id, HyMessageType t) {
	HyflowMessage dummy;
	std::map<unsigned long long, HyflowMessage*>::const_iterator ci;
	switch(t){
	case MSG_TRK_OBJECT:
		ci = trackerCallbackMap.find(m_id);
		if (ci == trackerCallbackMap.end())
			throw "Message not available";
		return *(ci->second);
	case MSG_ACCESS_OBJECT:
		ci = objCallbackMap.find(m_id);
		if (ci == objCallbackMap.end())
			throw "Message not available";
		return *(ci->second);
	default:
		throw "Invalid type message request to getbyId";
	}
	return dummy;
}
}

