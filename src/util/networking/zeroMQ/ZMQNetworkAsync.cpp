/*
 * ZMQNetworkAsync.cpp
 *	Async Version usages the Dealer and Router model
 *	No sendCallback supported directly.
 *
 *  Created on: Oct 5, 2012
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

#include "ZMQNetworkAsync.h"
#include "../NetworkManager.h"
#include "../../logging/Logger.h"
#include "../../messages/MessageHandler.h"
#include "../../concurrent/ThreadMeta.h"

namespace vt_dstm {

int ZMQNetworkAsync::nodeId = -1;
int ZMQNetworkAsync::basePort = -1;
int ZMQNetworkAsync::nodeCount = 0;

volatile bool ZMQNetworkAsync::hyflowShutdown = false;
int ZMQNetworkAsync::lingerTime = 0;

bool ZMQNetworkAsync::isInit = false;
int ZMQNetworkAsync::threadCount = 0;

std::vector<zmq::socket_t*> ZMQNetworkAsync::threadRouterSockets;
std::vector<zmq::socket_t*> ZMQNetworkAsync::recieverDealerSockets;
std::vector<pthread_t> ZMQNetworkAsync::dealerThreads;
std::vector<int*> ZMQNetworkAsync::dealerThreadIds;

ZMQNetworkAsync::ZMQNetworkAsync() {
	if (!isInit) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();
		threadCount = NetworkManager::getThreadCount();

		context = new zmq::context_t(1);
		// TODO: Move these sockets to transactional threads
		// Create sockets for each transactional thread
		for (int i=0 ; i < threadCount ; i++) {
			zmq::socket_t *threadSocket = new zmq::socket_t(*context, ZMQ_ROUTER);
			threadSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			std::stringstream threadStr;
			threadStr<<"tcp://"<<NetworkManager::getNodeIP()<<":"<<NetworkManager::getBasePort()+nodeId*threadCount+i;
			std::string threadAddr = threadStr.str();
			threadSocket->bind(threadAddr.c_str());
			threadRouterSockets.push_back(threadSocket);
		}

		// Create per node dedicated dealer sockets which process incoming requests
		// Set socket identity same for all the sockets as they talk to different nodes
		// TODO : Set socket Id, make sure stack value is OK
		std::stringstream idStr;
		idStr<<nodeId;
		std::string id = idStr.str();
		for (int i=0 ; i < nodeCount ; i++ ) {
			zmq::socket_t *recieverDealerSocket = new zmq::socket_t(*context, ZMQ_DEALER);
			recieverDealerSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			recieverDealerSocket->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
			recieverDealerSockets.push_back(recieverDealerSocket);
		}

		// Connect to Node 0
		for (int i=0; i<threadCount; i++) {
			std::stringstream baseStr;
			baseStr<<"tcp://"<<NetworkManager::getIp(0)<<":"<<NetworkManager::getBasePort()+i;
			std::string baseAddr = baseStr.str();
			recieverDealerSockets[0]->connect(baseAddr.c_str());
			if ( i == 0) {
				additionalSync();
			}
		}

		if (nodeId == 0) {
			// As all dealers are connected to required routers start the threads
			for (int i=0; i <nodeCount ; i++) {
				pthread_t dealerThread;
				int *waitingNodeId = new int();
				*waitingNodeId = i;
				dealerThreadIds.push_back(waitingNodeId);
				pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingNodeId);
				dealerThreads.push_back(dealerThread);
			}
		}else {
			// Create thread to receive any incoming message from Node 0
			pthread_t dealerThread;
			int *waitingNodeId = new int();
			*waitingNodeId = 0;
			dealerThreadIds.push_back(waitingNodeId);
			pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingNodeId);
			dealerThreads.push_back(dealerThread);
		}

		isInit = true;
	}
}

ZMQNetworkAsync::~ZMQNetworkAsync() {
	hyflowShutdown = true;

	for (unsigned int i=0 ; i <dealerThreads.size();i++) {
		pthread_kill(dealerThreads[i],SIGINT);
		pthread_join(dealerThreads[i], NULL);
		delete dealerThreadIds[i];
	}
	for (unsigned int i=0 ; i <recieverDealerSockets.size();i++) {
		if (recieverDealerSockets[i]) {
			zmq::socket_t* saveSocket = recieverDealerSockets[i];
			recieverDealerSockets[i] = NULL;
			delete saveSocket;
		}
	}
	for (unsigned int i = 0; i < threadRouterSockets.size();i++) {
		zmq::socket_t* saveSocket = threadRouterSockets[i];
		threadRouterSockets[i] = NULL;
		delete saveSocket;
	}
	if (context) {
		zmq::context_t* saveContext = context;
		context = NULL;
		delete saveContext;
	}
	LOG_DEBUG("Shutting Down ZeroMQ\n");
}

void ZMQNetworkAsync::additionalSync(){
	if (nodeId == 0) {
		// Wait on router to receive messages from dealers
		// All dealers are connected to node 0 only
		for (int i=1; i <nodeCount ; i++) {
			zmq::message_t msg_p1;
			LOG_DEBUG("ZMQA :Node 0 waiting for additional Sync Message\n");
			threadRouterSockets[0]->recv(&msg_p1);
			std::string senderId = std::string(static_cast<char*>(msg_p1.data()), msg_p1.size());
			zmq::message_t msg_p2;
			threadRouterSockets[0]->recv(&msg_p2);
			std::string senderIp = std::string(static_cast<char*>(msg_p2.data()), msg_p2.size());

			int sender = atoi(senderId.c_str());
			// Connect to sender's thread Routers
			for (int j=0 ; j<threadCount; j++) {
				std::stringstream baseStr;
				baseStr<<"tcp://"<<senderIp<<":"<<NetworkManager::getBasePort()+sender*threadCount+j;
				std::string baseAddr = baseStr.str();
				recieverDealerSockets[sender]->connect(baseAddr.c_str());
				LOG_DEBUG("ZMQA :Node 0 dealer %d connect to %s\n", sender, baseAddr.c_str());
			}
			sleep(1);
		}

		// Reply the sender nodes to continue
		for (int i=1; i<nodeCount ; i++) {
			std::string reply = std::string("Go");
			zmq::message_t zmqmsgBase(reply.size());
			memcpy(zmqmsgBase.data(), reply.data(), reply.size());

			// Add Address message part
			std::stringstream idStr;
			idStr<<i;
			std::string socketAddr = idStr.str();
			zmq::message_t zmqmsgAddr(socketAddr.size());
			memcpy(zmqmsgAddr.data(), socketAddr.data(), socketAddr.size());

			threadRouterSockets[0]->send(zmqmsgAddr, ZMQ_SNDMORE);
			threadRouterSockets[0]->send(zmqmsgBase);
		}
	}else {
		// Send your IP to node 0 through node 0 dealer
		std::string myIp = NetworkManager::getNodeIP();
		zmq::message_t msg(myIp.size());
		memcpy(msg.data(), myIp.data(), myIp.size());
		recieverDealerSockets[0]->send(msg);
		LOG_DEBUG("ZMQA :Additional Sync Message send to node 0\n");

		// Wait for response from node zero: Till when all routers are connected to node zero
		zmq::message_t rmsg;
		recieverDealerSockets[0]->recv(&rmsg);
		std::string node0Rep = std::string(static_cast<char*>(rmsg.data()), rmsg.size());
		LOG_DEBUG("ZMQA :Node 0 replied: %s\n", node0Rep.c_str());
	}
}

void ZMQNetworkAsync::networkInit(){
	// After first synchronization all IPs are available now connect to all nodes
	// For Node zero all connection are made on registration itself
	for (int i=1; (i < nodeCount) && (nodeId !=0); i++) {
		connectClient(i);

		// Create thread to receive any incoming message from Node i
		pthread_t dealerThread;
		int *waitingNodeId = new int();
		*waitingNodeId = i;
		dealerThreadIds.push_back(waitingNodeId);
		pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingNodeId);
		dealerThreads.push_back(dealerThread);
	}
//	// FIXME: Make sure all dealers are connected to routers to stop message drop
//	sleep (4);
	NetworkManager::synchronizeCluster();
}

void ZMQNetworkAsync::networkShutdown(){
	// Nothing is required to be done, we do clean up in destructor call
}

void ZMQNetworkAsync::connectClient(int toNodeId) {
	for (int i=0; i<threadCount; i++) {
		std::stringstream nodeStr;
		nodeStr<<"tcp://"<<NetworkManager::getIp(toNodeId)<<":"<<NetworkManager::getBasePort()+toNodeId*threadCount+i;
		recieverDealerSockets[toNodeId]->connect(nodeStr.str().c_str());
	}
}

void ZMQNetworkAsync::sendMessage(int toNodeId, HyflowMessage & message){
	// Serialize the Message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsgBase(msgData.size());
	memcpy(zmqmsgBase.data(), msgData.c_str(), msgData.size());

	// Add Address message part
	std::stringstream idStr;
	idStr<<toNodeId;
	std::string socketAddr = idStr.str();
	zmq::message_t zmqmsgAddr(socketAddr.size());
	memcpy(zmqmsgAddr.data(), socketAddr.data(), socketAddr.size());

	int threadId = ThreadMeta::getThreadId();
	LOG_DEBUG("ZMQA :Message send to %d using socket %d\n", toNodeId, threadId);
	threadRouterSockets[threadId]->send(zmqmsgAddr, ZMQ_SNDMORE);
	threadRouterSockets[threadId]->send(zmqmsgBase);
}

void ZMQNetworkAsync::sendCallbackMessage(int toNodeId, HyflowMessage & message, HyflowMessageFuture & fu){
	// As Async version don't support the callback we send all message one way
//	message.isCallback = false;

	// Serialize the message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsg(msgData.size());
	memcpy(zmqmsg.data(), msgData.c_str(), msgData.size());

	// Add Address message part
	std::stringstream idStr;
	idStr<<toNodeId;
	std::string socketAddr = idStr.str();
	zmq::message_t zmqmsgAddr(socketAddr.size());
	memcpy(zmqmsgAddr.data(), socketAddr.data(), socketAddr.size());

	int threadId = ThreadMeta::getThreadId();
	LOG_DEBUG("ZMQA :Message send to %d using socket %d\n", toNodeId, threadId);
	threadRouterSockets[threadId]->send(zmqmsgAddr, ZMQ_SNDMORE);
	threadRouterSockets[threadId]->send(zmqmsg);
	while (1){
		zmq::message_t zmqReply;
		threadRouterSockets[threadId]->recv(&zmqReply);
	    int64_t more;
	    size_t more_size = sizeof (more);
	    threadRouterSockets[threadId]->getsockopt(ZMQ_RCVMORE, &more, &more_size);
	    if (!more) {
			// Pass the last message part to callbackHandler
			callbackHandler(zmqReply);
	        break;      //  Last message frame
	    }
	}
}

bool ZMQNetworkAsync::defaultHandler(zmq::message_t & msg) {
	LOG_DEBUG("ZMQA :Got the Network Event \n");
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
			LOG_DEBUG("ZMQA :Received One way message\n");
			return false;
		}
	}
	return true;
}

void ZMQNetworkAsync::callbackHandler(zmq::message_t & msg){
	LOG_DEBUG("ZMQA :Got the Network Callback\n");
	if(msg.data() && (msg.size() > 0)) {
		std::string data((char*)msg.data(), msg.size());
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		LOG_DEBUG("ZMQA : Callback from node %d\n", req.toNode);
		if (req.msg_t != MSG_TYPE_INVALID)
			MessageHandler::callbackHandler(req);
	}
}

void* ZMQNetworkAsync::dealerExecute(void *param) {
	int id = *((int*)param);
	ThreadMeta::threadInit(/*id*/0, DISPATCHER_THREAD);
	s_catch_signals();
	LOG_DEBUG("ZMQA Server started\n");
	boost::posix_time::seconds sleepTime(0.0001);

	while (!hyflowShutdown) {
		try {
			while (1){		// Multipart message checking not required
				zmq::message_t request;
				recieverDealerSockets[id]->recv(&request);
				LOG_DEBUG("ZMQA :Dealer received Message\n");
			    int64_t more;
			    size_t more_size = sizeof (more);
			    recieverDealerSockets[id]->getsockopt(ZMQ_RCVMORE, &more, &more_size);
			    if (!more) {
			    	 //  Last message frame
					if (defaultHandler(request)) {
						recieverDealerSockets[id]->send(request);
					}
			        break;
			    }
			}
		} catch(zmq::error_t & e) {
			if (hyflowShutdown)
				break;
			else {
				throw e;
			}
		}
	}
	LOG_DEBUG("ZMQA Server Exiting\n");
	return NULL;
}

void ZMQNetworkAsync::s_catch_signals() {
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

void ZMQNetworkAsync::s_signal_handler(int signal_value) {
	LOG_DEBUG("ZMQA :Server Thread Signalled!!\n");
}

} /* namespace vt_dstm */
