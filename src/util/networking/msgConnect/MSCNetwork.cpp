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
#include <sched.h>

#include "MSCNetwork.h"
#include "../NetworkManager.h"
#include "../../logging/Logger.h"
#include "../../messages/MessageHandler.h"
#include "../../concurrent/ThreadId.h"

namespace vt_dstm
{
//int MSCNetwork::nodeCount = 0;
int MSCNetwork::nodeId = -1;
//int MSCNetwork::nodesInCluster = 0;

//std::string MSCNetwork::Ips[] = {
//		"10.1.1.20",
//		"10.1.1.21",
//		"10.1.1.22",
//		"10.1.1.24",
//		"10.1.1.25",
//		"10.1.1.26",
//		"10.1.1.27",
//		"10.1.1.28",
//		};
int MSCNetwork::basePort = -1;

//ConcurrentHashMap<HyMessageType, void (*)(HyflowMessage &)> MSCNetwork::handlerMap;
//ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::trackerCallbackMap;
//ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::objCallbackMap;
//ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::syncCallbackMap;
//ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::lockCallbackMap;
//ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::readValidCallbackMap;
//ConcurrentHashMap<unsigned long long, HyflowMessageFuture *> MSCNetwork::registerCallbackMap;
volatile bool MSCNetwork::hyflowShutdown = false;
boost::thread **MSCNetwork::dispatchThread = NULL;

//boost::condition MSCNetwork::onCluster;
//boost::mutex MSCNetwork::clsMutex;
//int MSCNetwork::syncVersion = false;
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
			dispatchThread[i] = new boost::thread(dispatcher, messenger, i+threadCount);
		}
		isInit = true;
	}
}

MSCNetwork::~MSCNetwork() {
	delete messenger;
	delete queue;
	delete socket;
	hyflowShutdown = true;
	for (int i=0 ; i < threadCount; i++ ) {
		dispatchThread[i]->join();
	}
	delete dispatchThread;
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
	socket->setInactivityTime(5000000);
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
//	int threadId = BenchmarkExecutor::getThreadId();
//	message.msg_id = getCurrentTime()*10000 + 100*toNodeId + threadId;	// Max 19 Digits
//	fu.setId(message.msg_id);
//	fu.setType(message.msg_t);
//
//	NetworkManager::registerMessageFuture(message.msg_id, message.msg_t, fu);

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

//void MSCNetwork::registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &)){
//	std::pair<HyMessageType, void (*)(HyflowMessage &)> p;
//	p.first = msg_t;
//	p.second = handlerFunc;
//	handlerMap.insertValue(p);
//}

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

//		LOG_DEBUG("MSNC : Event from node %d\n", req.fromNode);
//		// Handle Message
//		req.syncClocks();
//		void (*handler)(HyflowMessage &) = NULL;
//		handler = handlerMap.getValue(req.msg_t);
//		if ( handler != NULL)
//			handler(req);
//		else {
//			Logger::fatal("Message Handler Not available \n");
//		}

//		// Update from node clock
//		req.fromNodeClock = ContextManager::getClock();

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
//		req.syncClocks();
//		// FIXME: add try and catch block for possible exception
//		handlerMap.getValue(req.msg_t)(req);

		MessageHandler::callbackHandler(req);
	}
}

//std::string MSCNetwork::getIp(int id){
//	//FIXME: Complete the id dependent implementation
//	if (NetworkManager::islocalMachine())
//		return "127.0.0.1";
//	int mac = NetworkManager::getMachine();
//	return Ips[mac];
//}
//
//int MSCNetwork::getBasePort(){
//	return NetworkManager::getBasePort();
//}

//void MSCNetwork::registerMessageFuture(unsigned long long m_id, HyMessageType t, HyflowMessageFuture & fu) {
//	std::pair<unsigned long long, HyflowMessageFuture*> p;
//	p.first = m_id;
//	p.second = &fu;
//	switch (t)
//	{
//	case MSG_TRK_OBJECT:
//		// FIXME: add try and catch block for possible exception
//		trackerCallbackMap.insertValue(p);
//		break;
//	case MSG_ACCESS_OBJECT:
//		// FIXME: add try and catch block for possible exception
//		objCallbackMap.insertValue(p);
//		break;
//	case MSG_GRP_SYNC:
//		syncCallbackMap.insertValue(p);
//		break;
//	case MSG_LOCK_ACCESS:
//		lockCallbackMap.insertValue(p);
//		break;
//	case MSG_READ_VALIDATE:
//		readValidCallbackMap.insertValue(p);
//		break;
//	case MSG_REGISTER_OBJ:
//		registerCallbackMap.insertValue(p);
//		break;
//	default:
//		Logger::fatal("MSCN :registerMessageFuture :Invalid type message request to getbyId");
//		break;
//	}
//}
//
//HyflowMessageFuture & MSCNetwork::getMessageFuture(unsigned long long m_id, HyMessageType t) {
//	HyflowMessageFuture* future = NULL;
//	std::map<unsigned long long, HyflowMessageFuture*>::iterator i;
//
//	switch (t)
//	{
//	case MSG_TRK_OBJECT:
//		// FIXME: add try and catch block for possible exception
//		future = trackerCallbackMap.getValue(m_id);
//		break;
//	case MSG_ACCESS_OBJECT:
//		// FIXME: add try and catch block for possible exception
//		future = objCallbackMap.getValue(m_id);
//		break;
//	case MSG_GRP_SYNC:
//		future = syncCallbackMap.getValue(m_id);
//		break;
//	case MSG_LOCK_ACCESS:
//		future = lockCallbackMap.getValue(m_id);
//		break;
//	case MSG_READ_VALIDATE:
//		future = readValidCallbackMap.getValue(m_id);
//		break;
//	case MSG_REGISTER_OBJ:
//		future = registerCallbackMap.getValue(m_id);
//		break;
//	default:
//		Logger::fatal("MSCN :GetMessageFuture :Invalid type message request to getbyId");
//		break;
//	}
//	return *future;
//}
//
//void MSCNetwork::removeMessageFuture(unsigned long long m_id, HyMessageType t) {
//	std::map<unsigned long long, HyflowMessageFuture*>::iterator i;
//	switch(t){
//	case MSG_TRK_OBJECT:
//		// FIXME: add try and catch block for possible exception
//		trackerCallbackMap.deletePair(m_id);
//		break;
//	case MSG_ACCESS_OBJECT:
//		// FIXME: add try and catch block for possible exception
//		trackerCallbackMap.deletePair(m_id);
//		break;
//	case MSG_GRP_SYNC:
//		syncCallbackMap.deletePair(m_id);
//		break;
//	case MSG_LOCK_ACCESS:
//		lockCallbackMap.deletePair(m_id);
//		break;
//	case MSG_READ_VALIDATE:
//		readValidCallbackMap.deletePair(m_id);
//		break;
//	case MSG_REGISTER_OBJ:
//		registerCallbackMap.deletePair(m_id);
//		break;
//	default:
//		Logger::fatal("MSCN :RemoveMessageFuture :Invalid type message request to getbyId\n");
//		break;
//	}
//}

void MSCNetwork::dispatcher(MsgConnect::MCMessenger *mc, int dispatcherId) {
	LOG_DEBUG("Message Dispatcher started\n");
	boost::posix_time::seconds sleepTime(0.0001);
	ThreadId::setThreadId(dispatcherId);

	// Set the thread affinity
	cpu_set_t s;
	CPU_ZERO(&s);
	int node = NetworkManager::getNodeId()*threadCount + dispatcherId;
	CPU_SET(node, &s);
	sched_setaffinity(0, sizeof(cpu_set_t), &s);

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

//void MSCNetwork::waitTillSynchronized(int rqNo) {
//	LOG_DEBUG("MSNC : Starting wait for syncVer %d and ReqNo %d\n", syncVersion, rqNo);
//	boost::unique_lock<boost::mutex> lock(clsMutex);
//	while ( syncVersion != rqNo) {
//		onCluster.wait(lock);
//	}
//}

//void MSCNetwork::notifyCluster(int rqNo) {
//	//Reset the nodes In cluster count
//	nodesInCluster = 0;
//	{
//	     boost::unique_lock<boost::mutex> lock(clsMutex);
//	     syncVersion = rqNo;
//	 }
//	 onCluster.notify_all();
//	 LOG_DEBUG("MSNC : Notify all ReqNo %d\n", rqNo);
//}

//bool MSCNetwork::allNodeJoined(int rqNo) {
//	{
//		boost::unique_lock<boost::mutex> lock(clsMutex);
//		nodesInCluster++;
//	}
//	LOG_DEBUG("MSNC : Joining cluster in cluster %d in ReqNo %d\n", nodesInCluster, rqNo);
//	return nodesInCluster == nodeCount;
//}

//void MSCNetwork::replySynchronized(int rqNo) {
//	if (nodeId == 0) {
//		SynchronizeMsg gJmsg(nodeId, true, rqNo);
//		HyflowMessage hmsg;
//		hmsg.setMsg(&gJmsg);
//		hmsg.msg_t = MSG_GRP_SYNC;
//		hmsg.isCallback = false;
//		for (int i=0 ; i < nodeCount; i++)
//			sendMessage(i,hmsg);
//	}
//}

}

