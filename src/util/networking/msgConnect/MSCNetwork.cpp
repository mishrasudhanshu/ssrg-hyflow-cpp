/*
 * MCNetwork.cpp
 *
 *  Created on: Aug 16, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <string.h>

#include "MSCNetwork.h"
#include "../NetworkManager.h"
#include "../../logging/Logger.h"
#include "../../messages/MessageHandler.h"
#include "../../concurrent/ThreadMeta.h"

namespace vt_dstm
{
int MSCNetwork::nodeId = -1;
int MSCNetwork::basePort = -1;

volatile bool MSCNetwork::hyflowShutdown = false;
boost::thread **MSCNetwork::dispatchThread = NULL;

bool MSCNetwork::isInit = false;
int MSCNetwork::threadCount = 0;

MSCNetwork::MSCNetwork() {
	if (!isInit) {
//		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();

		messenger = new MsgConnect::MCMessenger();
		queue = new MsgConnect::MCQueue();
		socket = new MsgConnect::MCSocketTransport();
		threadCount = NetworkManager::getThreadCount();
		networkInit();
		LOG_DEBUG("Calling boost Dispatcher Thread\n");
		int dispThreads = threadCount;
		if (NetworkManager::getNodeCount() == 1) {
			dispThreads = 1;	// As everything done locally.
		}
		dispatchThread = new boost::thread*[dispThreads];
		for (int i=0; i < dispThreads ; i++) {
			dispatchThread[i] = new boost::thread(dispatcher, messenger, i);
		}
		isInit = true;
	}
}

MSCNetwork::~MSCNetwork() {
	if (messenger) {
		MsgConnect::MCMessenger* saveMessager = messenger;
		messenger = NULL;
		delete saveMessager;
	}
	if (queue) {
		MsgConnect::MCQueue* saveQueue = queue;
		queue = NULL;
		delete saveQueue;
	}
	if (socket) {
		MsgConnect::MCSocketTransport* saveSocket = socket;
		socket = NULL;
		delete saveSocket;
	}
	hyflowShutdown = true;
	for (int i=0 ; i < threadCount; i++ ) {
		dispatchThread[i]->join();
	}
	if (dispatchThread) {
		boost::thread **dp = dispatchThread;
		dispatchThread = NULL;
		delete dp;
	}
}

void MSCNetwork::networkInit(){
	std::string ipS = NetworkManager::getIp(nodeId);
	char *ip = (char*)ipS.c_str();
	unsigned int port = basePort + nodeId;
	std::stringstream qNameStr;
	qNameStr << nodeId << "-queue";
	std::string qString = qNameStr.str();
	char *qName = (char*)qString.c_str();

	messenger->setMaxTimeout(ULONG_MAX);
	socket->setActive(false);
	socket->setAttemptsToConnect(50);
	socket->setAttemptsInterval(100);	//Time is in MilliSecond
	socket->setFailOnInactive(true);
	socket->setInactivityTime(60000);	// If no response in 1 min on handshake retry
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
	LOG_DEBUG("Setup socket for ip %s : port %d : queue %s\n", ipS.c_str(), port, qString.c_str());
}

void MSCNetwork::sendMessage(int nodeId, HyflowMessage & message){
	std::stringstream destStr;
	destStr << "Socket:" << NetworkManager::getIp(nodeId) << ":" << basePort+nodeId << "|" << nodeId  <<"-queue";
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
	mcmsg.DataType = MsgConnect::bdtConst;

	mcmsg.Data = (void *) msg.c_str();
	mcmsg.DataSize = msg.size();

	messenger->PostMessage(destination.c_str(),&mcmsg,NULL);
}

void MSCNetwork::sendCallbackMessage(int toNodeId, HyflowMessage & message, HyflowMessageFuture & fu){
	std::stringstream destStr;
	destStr << "Socket:" << NetworkManager::getIp(toNodeId) << ":" << basePort+toNodeId << "|" << toNodeId  <<"-queue";
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
	LOG_DEBUG("Send a callback Message to %s\n", destination.c_str());
}

void MSCNetwork::defaultHandler(void* UserData, void* Sender,
	MsgConnect::MCMessage& msg, bool& Handled) {
	LOG_DEBUG("MSNC :Got the Network Event \n");
	if(msg.Data && (msg.DataSize > 0)) {
		// Read Message
		std::string idata((char*)msg.Data, msg.DataSize);
		std::istringstream idata_stream(idata);
		boost::archive::text_iarchive ia(idata_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		MessageHandler::msgHandler(req);

		// Pack handled message
		std::ostringstream odata_stream;
		boost::archive::text_oarchive oa(odata_stream);
		oa << req;
		std::string omsg = odata_stream.str();

		char *buffer = new char[omsg.size()];
		memcpy(buffer, omsg.c_str(), omsg.size());

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
	LOG_DEBUG("MSNC :Got the Network Callback\n");
	if(msg.Data && (msg.DataSize > 0)) {
		std::string data((char*)msg.Data, msg.DataSize);
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		LOG_DEBUG("MSNC : Callback from node %d\n", req.toNode);
		MessageHandler::callbackHandler(req);
	}
}

void MSCNetwork::dispatcher(MsgConnect::MCMessenger *mc, int dispatcherId) {
	LOG_DEBUG("Message Dispatcher started\n");
	boost::posix_time::seconds sleepTime(0.0001);
	ThreadMeta::threadInit(dispatcherId, DISPATCHER_THREAD);

	while (!hyflowShutdown) {
		try {
			mc->DispatchMessages();
		} catch (std::string & s) {
			Logger::fatal("%s\n",s.c_str());
			throw;
		}
		boost::this_thread::sleep(sleepTime);
	}
	LOG_DEBUG("Message Dispatcher shutdown\n");
}

}

