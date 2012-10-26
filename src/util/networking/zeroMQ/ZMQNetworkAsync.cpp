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
std::string* ZMQNetworkAsync::nodeIPs = NULL;

volatile bool ZMQNetworkAsync::hyflowShutdown = false;
int ZMQNetworkAsync::lingerTime = 0;

bool ZMQNetworkAsync::isInit = false;
int ZMQNetworkAsync::threadCount = 0;
zmq::socket_t* ZMQNetworkAsync::nodeInitSocket = NULL;
zmq::context_t* ZMQNetworkAsync::context=NULL;

volatile bool ZMQNetworkAsync::nodeReady = false;
boost::condition ZMQNetworkAsync::nodeReadyCondition;
boost::mutex ZMQNetworkAsync::nodeReadyMutex;

std::vector<zmq::socket_t*> ZMQNetworkAsync::threadRouterSockets;
std::vector<pthread_t> ZMQNetworkAsync::dealerThreads;
std::vector<int*> ZMQNetworkAsync::dealerThreadIds;

ZMQNetworkAsync::ZMQNetworkAsync() {
	if (!isInit) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();
		threadCount = NetworkManager::getThreadCount();

		nodeIPs = new std::string[nodeCount];
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
			LOG_DEBUG("ZMQA :Node 0 router %d bind to %s\n", i, threadAddr.c_str());
		}

		if (nodeId == 0) {
			nodeInitSocket = new zmq::socket_t(*context, ZMQ_ROUTER);
			nodeInitSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			std::stringstream threadStr;
			threadStr<<"tcp://"<<NetworkManager::getNodeIP()<<":"<<NetworkManager::getBasePort()-1;
			std::string threadAddr = threadStr.str();
			nodeInitSocket->bind(threadAddr.c_str());
		} else{
			nodeInitSocket = new zmq::socket_t(*context, ZMQ_DEALER);
			nodeInitSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			std::stringstream idStr;
			idStr<<nodeId;
			std::string id = idStr.str();
			nodeInitSocket->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
			std::stringstream threadStr;
			threadStr<<"tcp://"<<NetworkManager::getIp(0)<<":"<<NetworkManager::getBasePort()-1;
			std::string threadAddr = threadStr.str();
			nodeInitSocket->connect(threadAddr.c_str());
		}

		sleep(2);
		additionalSync();

		// Launch all the dealerExecutors
		for (int i=0; (i<threadCount) && nodeId; i++) {
			pthread_t dealerThread;
			int *waitingThreadId = new int();
			*waitingThreadId = i;
			dealerThreadIds.push_back(waitingThreadId);
			pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingThreadId);
			dealerThreads.push_back(dealerThread);
			sleep(1);
		}

		// Wait until last dealer thread is created and stabilised
		if (nodeId) {
			boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
			while ( !nodeReady )
				nodeReadyCondition.wait(nodeReadyMutex);
		}
		// After this much boiler code all nodes are able to communicate the synchronization messages
		isInit = true;
	}
}

ZMQNetworkAsync::~ZMQNetworkAsync() {
	hyflowShutdown = true;

	for (unsigned int i=0 ; i <dealerThreads.size();i++) {
		pthread_kill(dealerThreads[i],SIGINT);
		pthread_join(dealerThreads[i], NULL);
	}

	for (unsigned int i = 0; i < threadRouterSockets.size();i++) {
		zmq::socket_t* saveSocket = threadRouterSockets[i];
		threadRouterSockets[i] = NULL;
		delete saveSocket;
	}

	zmq::socket_t* tempSocket = nodeInitSocket;
	nodeInitSocket = NULL;
	delete tempSocket;

	if (context) {
		zmq::context_t* saveContext = context;
		context = NULL;
		delete saveContext;
	}
	LOG_DEBUG("Shutting Down ZeroMQ\n");
}

void ZMQNetworkAsync::additionalSync(){
	if (nodeId == 0) {
		nodeIPs[0] = NetworkManager::getIp(0);
		// Wait on router to receive messages from dealers
		// All dealers are connected to node 0 only
		for (int i=1; i<nodeCount ; i++) {
			zmq::message_t msg_p1;
			LOG_DEBUG("ZMQA :Node 0 waiting for additional Sync Message\n");
			nodeInitSocket->recv(&msg_p1);
			std::string senderId = std::string(static_cast<char*>(msg_p1.data()), msg_p1.size());
			zmq::message_t msg_p2;
			nodeInitSocket->recv(&msg_p2);
			std::string senderIp = std::string(static_cast<char*>(msg_p2.data()), msg_p2.size());

			int sender = atoi(senderId.c_str());
			nodeIPs[sender] = senderIp;
		}

		// As node zero has all IPs let it connect its dealer to all node routers
		for (int i=0; i<threadCount ; i++) {
			pthread_t dealerThread;
			int *waitingThreadId = new int();
			*waitingThreadId = i;
			dealerThreadIds.push_back(waitingThreadId);
			pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingThreadId);
			dealerThreads.push_back(dealerThread);
			sleep(1);		// Give time to dealer thread to stabilise
		}

		{// Wait for last thread to notify
			LOG_DEBUG("ZMQA : Waiting for all thread to notify\n");
			boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
			while (!nodeReady)
				nodeReadyCondition.wait(nodeReadyMutex);
		}

		// Prepare reply for nodes
		std::stringstream ipsStr;
		for (int i=0; i<nodeCount ; i++ ) {
			ipsStr<<nodeIPs[i]<<"|";
		}

		std::string reply = ipsStr.str();

		// Reply the sender nodes to continue
		for (int i=1; i<nodeCount ; i++) {
			zmq::message_t zmqmsgBase(reply.size());
			memcpy(zmqmsgBase.data(), reply.data(), reply.size());
			// Add Address message part
			std::stringstream idStr;
			idStr<<i;
			std::string socketAddr = idStr.str();
			zmq::message_t zmqmsgAddr(socketAddr.size());
			memcpy(zmqmsgAddr.data(), socketAddr.data(), socketAddr.size());

			nodeInitSocket->send(zmqmsgAddr, ZMQ_SNDMORE);
			nodeInitSocket->send(zmqmsgBase);
		}
	}else {
		// Send your IP to node 0 through node 0 dealer
		std::string myIp = NetworkManager::getNodeIP();
		zmq::message_t msg(myIp.size());
		memcpy(msg.data(), myIp.data(), myIp.size());
		nodeInitSocket->send(msg);
		LOG_DEBUG("ZMQA :Additional Sync Message send to node 0\n");

		// Wait for response from node zero: Till when all routers are connected to node zero
		zmq::message_t rmsg;
		nodeInitSocket->recv(&rmsg);
		std::string node0Rep = std::string(static_cast<char*>(rmsg.data()), rmsg.size());
		int ipStartIndex=0, lastDelimiter=0;
		for( int i=0 ; i<nodeCount ; i++ ) {
			int ipEndIndex = node0Rep.find('|',lastDelimiter);
			nodeIPs[i] = node0Rep.substr(ipStartIndex, ipEndIndex-ipStartIndex);
			ipStartIndex = ipEndIndex + 1;
			lastDelimiter = ipStartIndex;
			LOG_DEBUG("ZMQA : Read IP for node %d is %s\n", i, nodeIPs[i].c_str());
		}

		LOG_DEBUG("ZMQA :Node 0 replied: %s\n", node0Rep.c_str());
	}
}

void ZMQNetworkAsync::networkInit(){
	// Nothing to do
	sleep(2);
}

void ZMQNetworkAsync::networkShutdown(){
	// Nothing is required to be done, we do clean up in destructor call
}

void ZMQNetworkAsync::sendMessage(int toNodeId, HyflowMessage & message){
	// Serialize the Message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsgBase(msgData.size());
	memcpy(zmqmsgBase.data(), msgData.data(), msgData.size());

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
	// Serialize the message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsg(msgData.size());
	memcpy(zmqmsg.data(), msgData.data(), msgData.size());

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
		LOG_DEBUG("ZMQA : Got send callback response\n");
	    int64_t more = 1;
	    size_t more_size = sizeof (more);
	    threadRouterSockets[threadId]->getsockopt(ZMQ_RCVMORE, &more, &more_size);
	    if (!more) {
			// Pass the last message part to callbackHandler
	    	LOG_DEBUG("ZMQA : Handling callback response\n");
			callbackHandler(zmqReply);
	        break;      //  Last message frame
	    }
	}
}

bool ZMQNetworkAsync::defaultHandler(zmq::message_t & msg) {
	LOG_DEBUG("ZMQA :Got the Network Event \n");
	if(msg.data() && (msg.size() > 0)) {
		// Read Message
		std::string idata(static_cast<char*>(msg.data()), msg.size());
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
		std::string data(static_cast<char*>(msg.data()), msg.size());
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
	int waitThread = *((int*)param);

	ThreadMeta::threadInit(waitThread, DISPATCHER_THREAD);
	s_catch_signals();
	LOG_DEBUG("ZMQA Server started of thread %d\n", waitThread);
	boost::posix_time::seconds sleepTime(0.0001);

	std::vector<zmq::socket_t*> dealerSockets;

	//Create dealSockets and poll set and wait
	zmq::pollitem_t* nodeSet = new zmq::pollitem_t[nodeCount];
	std::stringstream idStr;
	idStr<<nodeId;
	std::string id = idStr.str();
	for (int i = 0 ; i<nodeCount ; i++) {
		zmq::socket_t *dealerSocket = new zmq::socket_t(*context, ZMQ_DEALER);
		dealerSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
		dealerSocket->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());

		std::stringstream baseStr;
		baseStr<<"tcp://"<<nodeIPs[i]<<":"<<NetworkManager::getBasePort()+i*threadCount+waitThread;
		std::string baseAddr = baseStr.str();
		dealerSocket->connect(baseAddr.c_str());
		dealerSockets.push_back(dealerSocket);

		nodeSet[i].socket = *dealerSocket;
		nodeSet[i].fd = 0;
		nodeSet[i].events = ZMQ_POLLIN;
		nodeSet[i].revents = 0;
	}

	sleep(2);

	// Last thread synchronization
	if (waitThread == threadCount-1) {
	    boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
	    nodeReady = true;
	    LOG_DEBUG("ZMQA : Last Thread Notifying the nodeReady\n");
	    nodeReadyCondition.notify_one();
	}

	while (!hyflowShutdown) {
		try {
			zmq::poll(nodeSet, nodeCount, -1);
			for (int i = 0 ; i< nodeCount ; i++) {
				if (nodeSet[i].revents & ZMQ_POLLIN) {
					while (1) {		// MultiPart message checking not required
						zmq::message_t request;
						zmq::socket_t *socket = dealerSockets[i];
						socket->recv(&request);
						LOG_DEBUG("ZMQA :Dealer received Message\n");
						int64_t more = 1;
						size_t more_size = sizeof (more);
						socket->getsockopt(ZMQ_RCVMORE, &more, &more_size);
						if (!more) {
							 //  Last message frame
							if (defaultHandler(request)) {
								LOG_DEBUG("ZMQA :Sending Callback Message Response\n");
								socket->send(request);
							}
							break;
						}
					}
					//Check on other sockets.
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

	for (unsigned int i = 0; i < dealerSockets.size();i++) {
		zmq::socket_t* saveSocket = dealerSockets[i];
		 dealerSockets[i] = NULL;
		delete saveSocket;
	}

	LOG_DEBUG("ZMQA Server Exiting for %d\n", waitThread);
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
