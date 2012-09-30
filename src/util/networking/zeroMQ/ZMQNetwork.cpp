/*
 * ZMQNetwork.cpp
 *
 *  Created on: Sep 29, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
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

bool ZMQNetwork::isInit = false;
int ZMQNetwork::threadCount = 0;

std::vector<zmq::socket_t*> ZMQNetwork::clientSockets;
std::vector<boost::mutex*> ZMQNetwork::socketMutexs;
zmq::socket_t * ZMQNetwork::serverSocket = NULL;
boost::thread *ZMQNetwork::serverThread = NULL;

ZMQNetwork::ZMQNetwork() {
	if (!isInit) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();

		context = new zmq::context_t(1);
		for (int i=0 ; i < nodeCount ; i++) {
			clientSockets.push_back(new zmq::socket_t(*context, ZMQ_REQ));
			socketMutexs.push_back(new boost::mutex());
		}
		// Create server socket
		serverSocket = new zmq::socket_t(*context, ZMQ_REP);
		std::stringstream serverStr;
		serverStr<<"tcp://"<<NetworkManager::getNodeIP()<<":"<<NetworkManager::getBasePort()+nodeId;
		serverSocket->bind(serverStr.str().c_str());
		LOG_DEBUG("ZMQ : Server started on %s\n", serverStr.str().c_str());

		// Connect to node zero
		std::stringstream clientStr;
		clientStr<<"tcp://"<<NetworkManager::getIp(0)<<":"<<NetworkManager::getBasePort();
		std::string clientAddr = clientStr.str();
		clientSockets[0]->connect(clientAddr.c_str());
		LOG_DEBUG("ZMQ :Connected to base Node %s\n", clientAddr.c_str());

		serverThread = new boost::thread(serverExecute);
		threadCount = NetworkManager::getThreadCount();
		isInit = true;
	}
}

ZMQNetwork::~ZMQNetwork() {
	for (unsigned int i = 0; i < clientSockets.size();i++) {
		zmq::socket_t* saveSocket = clientSockets[i];
		clientSockets[i] = NULL;
		delete saveSocket;
	}
	if (serverSocket) {
		zmq::socket_t* saveSocket = serverSocket;
		serverSocket = NULL;
		delete saveSocket;
	}
	if (context) {
		zmq::context_t* saveContext = context;
		context = NULL;
		delete saveContext;
	}
}

void ZMQNetwork::networkInit(){
	// After first synchronization all IPs are available now connect to all nodes
	// For Node zero all connection are made on registration itself
	for (int i=1; (i < nodeCount) && nodeId; i++) {
		connectClient(i);
	}
}

void ZMQNetwork::connectClient(int nodeId) {
	std::stringstream clientStr;
	clientStr<<"tcp://"<<NetworkManager::getIp(0)<<":"<<NetworkManager::getBasePort()+nodeId;
	clientSockets[nodeId]->connect(clientStr.str().c_str());
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

	clientSockets[toNodeId]->send(zmqmsg);
	zmq::message_t zmqReply;
	clientSockets[toNodeId]->recv(&zmqReply);
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

		if (req.msg_t != MSG_TYPE_INVALID)
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
		if (req.msg_t != MSG_TYPE_INVALID)
			MessageHandler::callbackHandler(req);
	}
}

void ZMQNetwork::serverExecute() {
	LOG_DEBUG("ZMQ Server started\n");
	boost::posix_time::seconds sleepTime(0.0001);

	while (!hyflowShutdown) {
		zmq::message_t request;
		serverSocket->recv(&request);
		if (defaultHandler(request)) {
			serverSocket->send(request);
		} else { // If we receive a non callback message return dummy reply
			HyflowMessage hmsg;
			hmsg.msg_t = MSG_TYPE_INVALID;
			std::ostringstream odata_stream;
			boost::archive::text_oarchive oa(odata_stream);
			oa << hmsg;
			std::string omsg = odata_stream.str();

			request.rebuild(omsg.size());
			memcpy(request.data(), omsg.c_str(), omsg.size());
			serverSocket->send(request);
		}
	}
}

} /* namespace vt_dstm */
