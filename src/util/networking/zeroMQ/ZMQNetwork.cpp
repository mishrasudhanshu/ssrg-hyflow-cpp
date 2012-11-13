/*
 * ZMQNetwork.cpp
 * Usages the REQ-REP pattern Synchronous design
 *  Created on: Sep 29, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <signal.h>
#include <stdint.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/thread/locks.hpp>

#include "ZMQNetwork.h"
#include "../NetworkManager.h"
#include "../../logging/Logger.h"
#include "../../messages/MessageHandler.h"
#include "../../concurrent/ThreadMeta.h"

namespace vt_dstm {

int ZMQNetwork::nodeId = -1;
int ZMQNetwork::basePort = -1;
int ZMQNetwork::nodeCount = 0;

volatile bool ZMQNetwork::hyflowShutdown = false;
int ZMQNetwork::lingerTime = 0;

bool ZMQNetwork::isInit = false;
int ZMQNetwork::threadCount = 0;

std::vector<zmq::socket_t*> ZMQNetwork::clientSockets;
std::vector<boost::mutex*> ZMQNetwork::socketMutexs;
std::vector<zmq::socket_t*> ZMQNetwork::serverSockets;
std::vector<pthread_t> ZMQNetwork::serverThreads;
std::vector<int*> ZMQNetwork::serverThreadIds;

ZMQNetwork::ZMQNetwork() {
	if (!isInit) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();
		threadCount = NetworkManager::getThreadCount();

		context = new zmq::context_t(1);
		for (int i=0 ; i < nodeCount ; i++) {
			zmq::socket_t *clientSocket = new zmq::socket_t(*context, ZMQ_REQ);
			clientSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			clientSockets.push_back(clientSocket);
			socketMutexs.push_back(new boost::mutex());
		}
		// Create server sockets we need to create the lock-up pair of sockets
		for (int i=0 ; i <nodeCount ; i++ ) {
			zmq::socket_t *serverSocket = new zmq::socket_t(*context, ZMQ_REP);
			serverSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			serverSockets.push_back(serverSocket);
			std::stringstream serverStr;
			serverStr<<"tcp://"<<NetworkManager::getNodeIP()<<":"<<NetworkManager::getBasePort()+nodeId*nodeCount +i;
			serverSocket->bind(serverStr.str().c_str());
			pthread_t serverThread;
			int *threadId = new int();
			*threadId = i;
			serverThreadIds.push_back(threadId);
			pthread_create(&serverThread,NULL,ZMQNetwork::serverExecute,(void*)threadId);
			serverThreads.push_back(serverThread);
			LOG_DEBUG("ZMQ : Server started on %s\n", serverStr.str().c_str());
		}

		// Connect to node zero
		std::stringstream clientStr;
		clientStr<<"tcp://"<<NetworkManager::getIp(0)<<":"<<NetworkManager::getBasePort()+nodeId;
		std::string clientAddr = clientStr.str();
		clientSockets[0]->connect(clientAddr.c_str());
		LOG_DEBUG("ZMQ :Connected to base Node %s\n", clientAddr.c_str());

		isInit = true;
	}
}

ZMQNetwork::~ZMQNetwork() {
	hyflowShutdown = true;

	for (int i=0 ; i <nodeCount;i++) {
		pthread_kill(serverThreads[i],SIGINT);
		pthread_join(serverThreads[i], NULL);
		delete serverThreadIds[i];
	}
	for (int i=0 ; i <nodeCount;i++) {
		if (serverSockets[i]) {
			zmq::socket_t* saveSocket = serverSockets[i];
			serverSockets[i] = NULL;
			delete saveSocket;
		}
	}
	for (unsigned int i = 0; i < clientSockets.size();i++) {
		zmq::socket_t* saveSocket = clientSockets[i];
		clientSockets[i] = NULL;
		delete saveSocket;
	}
	if (context) {
		zmq::context_t* saveContext = context;
		context = NULL;
		delete saveContext;
	}
	LOG_DEBUG("Shutting Down ZeroMQ\n");
}

void ZMQNetwork::networkInit(){
	// After first synchronization all IPs are available now connect to all nodes
	// For Node zero all connection are made on registration itself
	for (int i=1; (i < nodeCount) && nodeId; i++) {
		NetworkManager::registerNode(i);
	}
}

void ZMQNetwork::networkShutdown(){
	// Nothing is required to be done, we do clean up in destructor call
}

void ZMQNetwork::registerNode(int toNodeId) {
	std::stringstream clientStr;
	clientStr<<"tcp://"<<NetworkManager::getIp(toNodeId)<<":"<<NetworkManager::getBasePort()+toNodeId*nodeCount+nodeId;
	clientSockets[toNodeId]->connect(clientStr.str().c_str());
}

void ZMQNetwork::sendMessage(int toNodeId, HyflowMessage & message){
	// Serialize the Message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsg(msgData.size());
	memcpy(zmqmsg.data(), msgData.c_str(), msgData.size());

	// serialize the node socket access for multiple threads
	{
		boost::lock_guard<boost::mutex> socketGuard(*socketMutexs[toNodeId]);
		clientSockets[toNodeId]->send(zmqmsg);
		zmq::message_t zmqReply;
		clientSockets[toNodeId]->recv(&zmqReply);
		// Simply ignore the reply
	}
}

void ZMQNetwork::sendCallbackMessage(int toNodeId, HyflowMessage & message, HyflowMessageFuture & fu){
	// Serialize the message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsg(msgData.size());
	memcpy(zmqmsg.data(), msgData.c_str(), msgData.size());

	zmq::message_t zmqReply;
	{
		boost::lock_guard<boost::mutex> socketGuard(*socketMutexs[toNodeId]);
		clientSockets[toNodeId]->send(zmqmsg);
		clientSockets[toNodeId]->recv(&zmqReply);
	}
	callbackHandler(zmqReply);
}

bool ZMQNetwork::defaultHandler(zmq::message_t & msg) {
	LOG_DEBUG("ZMQ :Got the Network Event \n");
	if(msg.data() && (msg.size() > 0)) {
		// Read Message
		std::string idata((char*)msg.data(), msg.size());
		std::istringstream idata_stream(idata);
		boost::archive::text_iarchive ia(idata_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		if (req.msg_t != MSG_TYPE_DUMMY)
			MessageHandler::msgHandler(req);

		if (req.isCallback) {
			// Pack handled message
			std::ostringstream odata_stream;
			boost::archive::text_oarchive oa(odata_stream);
			oa << req;
			std::string omsg = odata_stream.str();

			msg.rebuild(omsg.size());
			memcpy(msg.data(), omsg.c_str(), omsg.size());
			return true;
		}else {
			LOG_DEBUG("ZMQ :Received One way message\n");
			return false;
		}
	}
	return true;
}

void ZMQNetwork::callbackHandler(zmq::message_t & msg){
	LOG_DEBUG("ZMQ :Got the Network Callback\n");
	if(msg.data() && (msg.size() > 0)) {
		std::string data((char*)msg.data(), msg.size());
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		LOG_DEBUG("ZMQ : Callback from node %d\n", req.toNode);
		if (req.msg_t != MSG_TYPE_DUMMY)
			MessageHandler::callbackHandler(req);
	}
}

void* ZMQNetwork::serverExecute(void *param) {
	int id = *((int*)param);
	ThreadMeta::threadInit(id, DISPATCHER_THREAD);
	s_catch_signals();
	LOG_DEBUG("ZMQ Server started\n");
	boost::posix_time::seconds sleepTime(0.0001);

	while (!hyflowShutdown) {
		zmq::message_t request;
		try {
			serverSockets[id]->recv(&request);
		} catch(zmq::error_t & e) {
			if (hyflowShutdown)
				break;
			else {
				throw e;
			}
		}
		if (defaultHandler(request)) {
			try {
				serverSockets[id]->send(request);
			} catch(zmq::error_t & e) {
				if (hyflowShutdown)
					break;
				else {
					throw e;
				}
			}
		} else { // If we receive a non callback message return dummy reply
			HyflowMessage hmsg;
			hmsg.msg_t = MSG_TYPE_DUMMY;
			std::ostringstream odata_stream;
			boost::archive::text_oarchive oa(odata_stream);
			oa << hmsg;
			std::string omsg = odata_stream.str();

			request.rebuild(omsg.size());
			memcpy(request.data(), omsg.c_str(), omsg.size());
			try {
				serverSockets[id]->send(request);
			} catch(zmq::error_t & e) {
				if (hyflowShutdown)
					break;
				else {
					throw e;
				}
			}
		}
	}
	LOG_DEBUG("ZMQ Server Exiting\n");
	return NULL;
}

void ZMQNetwork::s_catch_signals() {
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

void ZMQNetwork::s_signal_handler(int signal_value) {
	LOG_DEBUG("ZMQ Server Thread Signalled!!\n");
}

} /* namespace vt_dstm */
