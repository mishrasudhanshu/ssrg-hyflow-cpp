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
#include "../../logging/Logger.h"

namespace vt_dstm
{
int MSCNetwork::nodeCount = 0;
int MSCNetwork::nodeId = -1;
int MSCNetwork::nodesInCluster = 0;

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

ConcurrentHashMap<HyMessageType, void (*)(HyflowMessage &)> MSCNetwork::handlerMap;
ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::trackerCallbackMap;
ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::objCallbackMap;
ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::syncCallbackMap;
volatile bool MSCNetwork::hyflowShutdown = false;
boost::thread *MSCNetwork::dispatchThread = NULL;

boost::condition MSCNetwork::onCluster;
boost::mutex MSCNetwork::clsMutex;
int MSCNetwork::syncVersion = false;
bool MSCNetwork::isInit = false;

MSCNetwork::MSCNetwork() {
	if (!isInit) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();
		nodeIps = new std::string[nodeCount];

		messenger = new MsgConnect::MCMessenger();
		queue = new MsgConnect::MCQueue[nodeCount];
		socket = new MsgConnect::MCSocketTransport[nodeCount];
		setupSockets();
		Logger::debug("Calling boost Dispatcher Thread\n");
		dispatchThread = new boost::thread(dispatcher, messenger);
		isInit = true;
	}
}

MSCNetwork::~MSCNetwork() {
	delete messenger;
	delete[] queue;
	delete[] socket;
	hyflowShutdown = true;
	dispatchThread->join();
	delete dispatchThread;
}

void MSCNetwork::setupSockets(){
	for(int i=0 ; i<nodeCount ; i++){
		nodeIps[i] = getIp(i);
		std::string ipS = getIp(i);
		char *ip = (char*)ipS.c_str();
		unsigned int port = basePort + nodeId;
		std::stringstream qNameStr;
		qNameStr << nodeId << "-queue";
		std::string qString = qNameStr.str();
		char *qName = (char*)qString.c_str();

		messenger->setMaxTimeout(ULONG_MAX);
		socket->setActive(false);
		socket->setAttemptsToConnect(1);
		socket->setFailOnInactive(true);
		socket->setMaxTimeout(900000l);

		socket->setMessengerAddress(ip);
		socket->setMessengerPort(port);
		socket->setTransportMode(stmP2P);
		socket->setMessenger(messenger);
		socket->setActive(true);

		queue->setQueueName(qName);
		queue->setMessenger(messenger);

		MsgConnect::MCMessageHandlers *mhls = queue->getHandlers();
		MsgConnect::MCMessageHandler* handler = mhls->Add();
		handler->setMsgCodeLow(1);
		handler->setMsgCodeHigh(1);
		handler->setOnMessage(MSCNetwork::defaultHandler);
		handler->setEnabled(true);
		Logger::debug("Setup socket for ip %s : port %d : queue %s\n", ipS.c_str(), port, qString.c_str());
	}
}

void MSCNetwork::sendMessage(int nodeId, HyflowMessage & message){
	std::stringstream destStr;
	destStr << "Socket:" << getIp(nodeId) << ":" << getBasePort()+nodeId << "|" << nodeId  <<"-queue";
	std::string destination = destStr.str();

	// Serialize the Message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msg = ostream.str();

	MsgConnect::MCMessage mcmsg;
	mcmsg.MsgCode = 1;
	mcmsg.Param1 = 0;
	mcmsg.Param2 = 0;
	mcmsg.DataType = MsgConnect::bdtVar;

	mcmsg.Data = (void *) msg.c_str();
	mcmsg.DataSize = msg.size();

	messenger->PostMessage(destination.c_str(),&mcmsg,NULL);
}

void MSCNetwork::sendCallbackMessage(int toNodeId, HyflowMessage & message, HyflowMessageFuture & fu){
	message.msg_id = getCurrentTime()*1000 + toNodeId;
	fu.setId(message.msg_id);
	fu.setType(message.msg_t);

	NetworkManager::registerMessageFuture(message.msg_id, message.msg_t, fu);

	std::stringstream destStr;
	destStr << "Socket:" << getIp(toNodeId) << ":" << getBasePort()+toNodeId << "|" << toNodeId  <<"-queue";
	std::string destination = destStr.str();

	// Serialize the message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	MsgConnect::MCMessage mcmsg;
	mcmsg.MsgCode = 1;
	mcmsg.Param1 = 0;
	mcmsg.Param2 = 0;
	mcmsg.DataType = MsgConnect::bdtVar;

	mcmsg.Data = (void *) msgData.c_str();
	mcmsg.DataSize = msgData.size();

	messenger->SendMessageCallback(destination.c_str(), &mcmsg, &callbackHandler,0,NULL);
	Logger::debug("Send a callback Message to %s\n", destination.c_str());
}

void MSCNetwork::registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &)){
	std::pair<HyMessageType, void (*)(HyflowMessage &)> p;
	p.first = msg_t;
	p.second = handlerFunc;
	handlerMap.insertValue(p);
}

void MSCNetwork::defaultHandler(void* UserData, void* Sender,
	MsgConnect::MCMessage& msg, bool& Handled) {
	Logger::debug("Got the Network Event\n");
	if(msg.Data && (msg.DataSize > 0)) {
		// Read Message
		std::string idata((char*)msg.Data, msg.DataSize);
		std::istringstream idata_stream(idata);
		boost::archive::text_iarchive ia(idata_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		// Handle Message
		void (*handler)(HyflowMessage &) = NULL;
		handler = handlerMap.getValue(req.msg_t);
		if ( handler != NULL)
			handler(req);
		else {
			Logger::fatal("Message Handler Not available \n");
		}
		// Pack handled message
		std::ostringstream odata_stream;
		boost::archive::text_oarchive oa(odata_stream);
		oa << req;
		std::string omsg = odata_stream.str();

		char *buffer = new char[omsg.size()];
		std::memcpy(buffer, omsg.c_str(), omsg.size());

		//Do it safely Delete buffer pointing to older message
		void* oldData = msg.Data;
		msg.Data = (void *)buffer;
		msg.DataSize = omsg.size();

		// LESSON : In Msgconnect uses malloc to allocation oldData
		MsgConnect::MCMemFree(oldData);
		Handled = true;
	}
}

void MSCNetwork::callbackHandler(unsigned int UserData, MsgConnect::MCMessage& msg){
	Logger::debug("Got the Network Callback\n");
	if(msg.Data && (msg.DataSize > 0)) {
		std::string data((char*)msg.Data, msg.DataSize);
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		// FIXME: add try and catch block for possible exception
		handlerMap.getValue(req.msg_t)(req);
	}
}

std::string MSCNetwork::getIp(int id){
	//FIXME: Complete the id dependent implementation
	if (NetworkManager::islocalMachine())
		return "127.0.0.1";
	int mac = NetworkManager::getMachine();
	return Ips[mac];
}

int MSCNetwork::getBasePort(){
	return NetworkManager::getBasePort();
}

unsigned long long MSCNetwork::getCurrentTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}

void MSCNetwork::registerMessageFuture(unsigned long long m_id, HyMessageType t, HyflowMessageFuture & fu) {
	std::pair<unsigned long long, HyflowMessageFuture*> p;
	p.first = m_id;
	p.second = &fu;
	switch (t)
	{
	case MSG_TRK_OBJECT:
		// FIXME: add try and catch block for possible exception
		trackerCallbackMap.insertValue(p);
		break;
	case MSG_ACCESS_OBJECT:
		// FIXME: add try and catch block for possible exception
		objCallbackMap.insertValue(p);
		break;
	case MSG_GRP_SYNC:
		syncCallbackMap.insertValue(p);
		break;
	default:
		Logger::fatal("MSCN :registerMessageFuture :Invalid type message request to getbyId");
		break;
	}
}

HyflowMessageFuture & MSCNetwork::getMessageFuture(unsigned long long m_id, HyMessageType t) {
	HyflowMessageFuture* future = NULL;
	std::map<unsigned long long, HyflowMessageFuture*>::iterator i;

	switch (t)
	{
	case MSG_TRK_OBJECT:
		// FIXME: add try and catch block for possible exception
		future = trackerCallbackMap.getValue(m_id);
		break;
	case MSG_ACCESS_OBJECT:
		// FIXME: add try and catch block for possible exception
		future = objCallbackMap.getValue(m_id);
		break;
	case MSG_GRP_SYNC:
		future = syncCallbackMap.getValue(m_id);
		break;
	default:
		Logger::fatal("MSCN :GetMessageFuture :Invalid type message request to getbyId");
		break;
	}
	return *future;
}

void MSCNetwork::removeMessageFuture(unsigned long long m_id, HyMessageType t) {
	std::map<unsigned long long, HyflowMessageFuture*>::iterator i;
	switch(t){
	case MSG_TRK_OBJECT:
		// FIXME: add try and catch block for possible exception
		trackerCallbackMap.deletePair(m_id);
		break;
	case MSG_ACCESS_OBJECT:
		// FIXME: add try and catch block for possible exception
		trackerCallbackMap.deletePair(m_id);
		break;
	case MSG_GRP_SYNC:
		syncCallbackMap.deletePair(m_id);
		break;
	default:
		Logger::fatal("MSCN :RemoveMessageFuture :Invalid type message request to getbyId\n");
		break;
	}
}

void MSCNetwork::dispatcher(MsgConnect::MCMessenger *mc) {
	Logger::debug("Message Dispatcher started\n");
	boost::posix_time::seconds sleepTime(0.0005);
	while (!hyflowShutdown) {
		mc->DispatchMessages();
		boost::this_thread::sleep(sleepTime);
	}
}

void MSCNetwork::waitTillSynchronized(int rqNo) {
	Logger::debug("MSNC : Starting wait for synchronization\n");
	boost::unique_lock<boost::mutex> lock(clsMutex);
	while ( syncVersion != rqNo) {
		onCluster.wait(lock);
	}
}

void MSCNetwork::notifyCluster(int rqNo) {
	//Reset the nodes In cluster count
	nodesInCluster = 0;
	{
	     boost::unique_lock<boost::mutex> lock(clsMutex);
	     syncVersion = rqNo;
	 }
	 onCluster.notify_all();
}

bool MSCNetwork::allNodeJoined(int rqNo) {
	Logger::debug("MSNC : Joining cluster\n");
	{
		boost::unique_lock<boost::mutex> lock(clsMutex);
		nodesInCluster++;
	}
	return nodesInCluster == nodeCount;
}

void MSCNetwork::setSynchronized(int rqNo) {
	Logger::debug("MSNC : synchronize notification received\n");
	notifyCluster(rqNo);
}

}
