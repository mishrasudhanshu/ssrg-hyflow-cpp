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
//TODO: Create node socket and don't use socket 0 for communication
int ZMQNetworkAsync::nodeId = -1;
int ZMQNetworkAsync::basePort = -1;
int ZMQNetworkAsync::nodeCount = 0;
int ZMQNetworkAsync:: dealerThreadPerNode = 0;

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
		dealerThreadPerNode = threadCount/nodeCount;

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
		for (int i=0 ; i < nodeCount*threadCount ; i++ ) {
			zmq::socket_t *recieverDealerSocket = new zmq::socket_t(*context, ZMQ_DEALER);
			recieverDealerSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			recieverDealerSocket->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
			recieverDealerSockets.push_back(recieverDealerSocket);
		}

		// Connect my node dealers to routers sockets of Node 0
		for (int i=0; i<threadCount; i++) {
			std::stringstream baseStr;
			baseStr<<"tcp://"<<NetworkManager::getIp(0)<<":"<<NetworkManager::getBasePort()+i;
			std::string baseAddr = baseStr.str();
			recieverDealerSockets[i]->connect(baseAddr.c_str());
			if ( i==0 ) {
				additionalSync();
			}
		}

		if (nodeId == 0) {
			// As all node 0 dealers are connected to all nodes routers, start the dealer threads for node 0
			for (int i=0; i <nodeCount ; i++) {
				if (dealerThreadPerNode > 1) {
					for(int msgThread = 0; msgThread < dealerThreadPerNode; msgThread++) {
						pthread_t dealerThread;
						int *dealerThreadArgs = new int[2];
						dealerThreadArgs[0] = i;
						dealerThreadArgs[1] = msgThread;
						dealerThreadIds.push_back(dealerThreadArgs);
						pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)dealerThreadArgs);
						dealerThreads.push_back(dealerThread);
					}
				}else {
					pthread_t dealerThread;
					int *waitingNodeId = new int();
					*waitingNodeId = i;
					dealerThreadIds.push_back(waitingNodeId);
					pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingNodeId);
					dealerThreads.push_back(dealerThread);
				}
			}
		}else {
			if (dealerThreadPerNode > 1) {
				for(int msgThread = 0; msgThread < dealerThreadPerNode; msgThread++) {
					// Create threads to receive any incoming message from Node 0
					pthread_t dealerThread;
					int *dealerThreadArgs = new int[2];
					dealerThreadArgs[0] = 0;
					dealerThreadArgs[1] = msgThread;
					dealerThreadIds.push_back(dealerThreadArgs);
					pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)dealerThreadArgs);
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
		if (dealerThreadPerNode > 1)
			delete [] dealerThreadIds[i];
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
				recieverDealerSockets[sender*threadCount+j]->connect(baseAddr.c_str());
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
		sleep(1);
	}
}

void ZMQNetworkAsync::networkInit(){
	// After first synchronization all IPs are available now connect to all nodes
	// For Node zero all connection are made on registration itself
	for (int i=1; (i < nodeCount) && (nodeId !=0); i++) {
		connectClient(i);

		if (dealerThreadPerNode > 1) {
			// Create threads to receive any incoming message from Node i
			for(int msgThread = 0; msgThread < dealerThreadPerNode; msgThread++) {
				pthread_t dealerThread;
				int *dealerThreadArgs = new int[2];
				dealerThreadArgs[0] = i;
				dealerThreadArgs[1] = msgThread;
				dealerThreadIds.push_back(dealerThreadArgs);
				pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)dealerThreadArgs);
				dealerThreads.push_back(dealerThread);
			}
		}else {
			pthread_t dealerThread;
			int *waitingNodeId = new int();
			*waitingNodeId = i;
			dealerThreadIds.push_back(waitingNodeId);
			pthread_create(&dealerThread,NULL,ZMQNetworkAsync::dealerExecute,(void*)waitingNodeId);
			dealerThreads.push_back(dealerThread);
		}
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
		recieverDealerSockets[toNodeId*threadCount+i]->connect(nodeStr.str().c_str());
	}
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
	int id = *((int*)param);
	int partitions = 0;
	int firstNodeIndex = 0;
	int lastNodeIndex = threadCount;

	if (dealerThreadPerNode > 1) {
		partitions = *((int*)param+1);
		firstNodeIndex = nodeCount*partitions;
		lastNodeIndex = nodeCount*partitions + nodeCount;
		if (partitions == dealerThreadPerNode -1) {
			lastNodeIndex = threadCount;
		}
	}
	ThreadMeta::threadInit(id, DISPATCHER_THREAD);
	s_catch_signals();
	LOG_DEBUG("ZMQA Server started\n");
	boost::posix_time::seconds sleepTime(0.0001);

	/*
	 * Currently creating one thread to process all incoming message
	 * from a given node id. nodeSet contains the dealer sockets
	 * corresponding to all the threads on particular node.
	 */

	//Create dealSocket poll set and wait
	int setSize = lastNodeIndex - firstNodeIndex;
	zmq::pollitem_t* nodeSet = new zmq::pollitem_t[setSize];
	int firstNodeIndexInDealers = id*threadCount+firstNodeIndex;
	for (int i = 0 ; i<setSize ; i++) {
		nodeSet[i].socket = *recieverDealerSockets[firstNodeIndexInDealers+i];
		nodeSet[i].fd = 0;
		nodeSet[i].events = ZMQ_POLLIN;
		nodeSet[i].revents = 0;
	}

	while (!hyflowShutdown) {
		try {
			zmq::poll(nodeSet, setSize, -1);
			for (int i = 0 ; i<setSize ; i++) {
				if (nodeSet[i].revents & ZMQ_POLLIN) {
					while (1) {		// Multipart message checking not required
						zmq::message_t request;
						zmq::socket_t *socket = recieverDealerSockets[firstNodeIndexInDealers+i];
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