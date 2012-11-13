/*
 * ZMQNetworkAsyncPollPoll.cpp
 *
 *  Created on: Nov 10, 2012
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

#include "ZMQNetworkAsyncPoll.h"
#include "../NetworkManager.h"
#include "../../logging/Logger.h"
#include "../../messages/MessageHandler.h"
#include "../../concurrent/ThreadMeta.h"
#include "../../parser/ConfigFile.h"
#include "../../Definitions.h"

#define WORKER_STR "worker"

namespace vt_dstm {

int ZMQNetworkAsyncPoll::nodeId = -1;
int ZMQNetworkAsyncPoll::basePort = -1;
int ZMQNetworkAsyncPoll::nodeCount = 0;
int ZMQNetworkAsyncPoll::zeroMQTFR = 0;
int ZMQNetworkAsyncPoll::zeroMQWFR = 0;
int ZMQNetworkAsyncPoll::forwardersNcatchers= 0;
int ZMQNetworkAsyncPoll::catcherWorkers = 0;

std::string* ZMQNetworkAsyncPoll::nodeIPs = NULL;

volatile bool ZMQNetworkAsyncPoll::hyflowShutdown = false;
int ZMQNetworkAsyncPoll::lingerTime = 0;

bool ZMQNetworkAsyncPoll::isInit = false;
int ZMQNetworkAsyncPoll::threadCount = 0;
zmq::socket_t* ZMQNetworkAsyncPoll::nodeInitSocket = NULL;
zmq::socket_t* ZMQNetworkAsyncPoll::mainThreadSocket = NULL;
zmq::context_t* ZMQNetworkAsyncPoll::context=NULL;

volatile bool ZMQNetworkAsyncPoll::nodeReady = false;
boost::condition ZMQNetworkAsyncPoll::nodeReadyCondition;
boost::mutex ZMQNetworkAsyncPoll::nodeReadyMutex;

std::vector<zmq::socket_t*> ZMQNetworkAsyncPoll::threadRouterSockets;
std::vector<pthread_t> ZMQNetworkAsyncPoll::forwarderThreads;
std::vector<pthread_t> ZMQNetworkAsyncPoll::catcherThreads;
pthread_t ZMQNetworkAsyncPoll::ForwarderThread;
std::vector<int*> ZMQNetworkAsyncPoll::forwarderThreadIds;
std::vector<int*> ZMQNetworkAsyncPoll::catcherThreadIds;
boost::thread_specific_ptr<zmq::socket_t> ZMQNetworkAsyncPoll::thread_socket;

ZMQNetworkAsyncPoll::ZMQNetworkAsyncPoll() {
	if (!isInit) {
		nodeCount = NetworkManager::getNodeCount();
		nodeId = NetworkManager::getNodeId();
		basePort = NetworkManager::getBasePort();
		threadCount = NetworkManager::getThreadCount();

		nodeIPs = new std::string[nodeCount];
		context = new zmq::context_t(1);
		zeroMQTFR = atoi(ConfigFile::Value(ZERO_MQ_TFR).c_str());
		zeroMQWFR = atoi(ConfigFile::Value(ZERO_MQ_WFR).c_str());
		forwardersNcatchers = threadCount/zeroMQTFR;
		catcherWorkers = zeroMQWFR;
		sleep(2);

		// Create Forwarder and Catcher threads for this node
		for (int i=0 ; i< forwardersNcatchers ; i++ ) {
			int* tid = new int();
			*tid = i;
			pthread_t fthread;
			pthread_create(&fthread,NULL,ZMQNetworkAsyncPoll::forwarderThread, tid);
			forwarderThreads.push_back(fthread);
		}

		// Wait for last Forwarder thread to signal back
		{
			boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
			while ( !nodeReady )
				nodeReadyCondition.wait(nodeReadyMutex);
		}
		nodeReady = false;

		// We come here via main thread call, good time to set its id to threadCount
		ThreadMeta::threadInit(threadCount, MAIN_THREAD);

		// Communicate on base socket and collect all the IPs
		if (nodeId == 0) {
			nodeInitSocket = new zmq::socket_t(*context, ZMQ_ROUTER);
			nodeInitSocket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			std::stringstream threadStr;
			threadStr<<"tcp://"<<NetworkManager::getNodeIP()<<":"<<NetworkManager::getBasePort()-1;
			std::string threadAddr = threadStr.str();
			nodeInitSocket->bind(threadAddr.c_str());
		} else{
			sleep(2);
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

		// Create Catcher threads for this node
		if ( nodeId ) {
			for (int i=0 ; i<forwardersNcatchers ; i++ ) {
				int* tid = new int();
				*tid = i;
				pthread_t cthread;
				pthread_create(&cthread,NULL,ZMQNetworkAsyncPoll::catcherThread, tid);
				catcherThreads.push_back(cthread);
			}

			// Wait for last catcher thread to signal back
			{
				boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
				while ( !nodeReady )
					nodeReadyCondition.wait(nodeReadyMutex);
			}
			nodeReady = false;
		}
		sleep(2);

		// After this much boiler code all nodes are able to communicate the synchronization messages
		isInit = true;
	}
}

ZMQNetworkAsyncPoll::~ZMQNetworkAsyncPoll() {
	hyflowShutdown = true;
	for (unsigned int catchererCount=0 ; catchererCount < catcherThreads.size(); catchererCount++) {
		pthread_kill(catcherThreads[catchererCount],SIGINT);
		pthread_join(catcherThreads[catchererCount], NULL);
	}
	LOG_DEBUG("ZMQA :Catcher threads killed\n");
	sleep(2);

	for (unsigned int forwarderCount=0 ; forwarderCount <forwarderThreads.size();forwarderCount++) {
		pthread_kill(forwarderThreads[forwarderCount],SIGINT);
		pthread_join(forwarderThreads[forwarderCount], NULL);
	}
	LOG_DEBUG("ZMQA :Forwarder threads killed\n");
	sleep(2);

//	for (unsigned int i = 0; i < threadRouterSockets.size();i++) {
//		zmq::socket_t* saveSocket = threadRouterSockets[i];
//		threadRouterSockets[i] = NULL;
//		delete saveSocket;
//	}


	zmq::socket_t* tempSocket = nodeInitSocket;
	nodeInitSocket = NULL;
	delete tempSocket;

	tempSocket = mainThreadSocket;
	mainThreadSocket = NULL;
	delete tempSocket;

	if (context) {
		zmq::context_t* saveContext = context;
		context = NULL;
		//TODO: do proper clean-up, some issue due to threadLocal storage
		delete saveContext;
	}
	LOG_DEBUG("Shutting Down ZeroMQ\n");
}

void ZMQNetworkAsyncPoll::additionalSync(){
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

		// As node zero has all IPs lets create all of its catcher
		for (int i=0 ; i<forwardersNcatchers ; i++ ) {
			int* tid = new int();
			*tid = i;
			pthread_t cthread;
			pthread_create(&cthread,NULL,ZMQNetworkAsyncPoll::catcherThread, tid);
			catcherThreads.push_back(cthread);
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

void ZMQNetworkAsyncPoll::networkInit(){
	// Nothing to do
	sleep(2);
}

void ZMQNetworkAsyncPoll::networkShutdown(){
	// Nothing is required to be done, we do clean up in destructor call
}

void ZMQNetworkAsyncPoll::threadNetworkInit() {
	getThreadSocket();
}

void ZMQNetworkAsyncPoll::threadNetworkShutdown() {
	zmq::socket_t *socket = NULL;
	socket = thread_socket.get();
	if (socket) {
		thread_socket.reset(NULL);
		LOG_DEBUG("ZMQAP : Going to Delete thread Specific socket\n");
		delete socket;
	}
}

zmq::socket_t* ZMQNetworkAsyncPoll::getThreadSocket() {
	zmq::socket_t* socket = thread_socket.get();
	if (!socket) {
		int threadId = ThreadMeta::getThreadId();
		if (threadId == -1 ) {
			Logger::fatal("ZMQAP : This thread should not send message\n");
		}else{
			std::stringstream idStr;
			idStr<<threadId;
			std::string id = idStr.str();
			std::stringstream threadStr;
			threadStr<<"inproc://forwarder-"<<threadId%forwardersNcatchers;
			std::string forwarderAddr = threadStr.str();

			socket = new zmq::socket_t(*context, ZMQ_DEALER);
			socket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
			socket->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
			socket->connect(forwarderAddr.c_str());

			threadRouterSockets.push_back(socket);
			LOG_DEBUG("ZMQA :Thread %s connected to %s\n", id.c_str(), forwarderAddr.c_str());
			thread_socket.reset(socket);
			if(threadId == threadCount) { // Main Thread
				mainThreadSocket = socket;
			}
			sleep(2);
		}
	}
	return socket;
}

void ZMQNetworkAsyncPoll::sendMessage(int toNodeId, HyflowMessage & message){
	// Serialize the Message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsgBase(msgData.size());
	memcpy(zmqmsgBase.data(), msgData.data(), msgData.size());

	// Add Address message part
	std::stringstream idStr;
	idStr<<"catcher-"<<toNodeId<<"-"<<ThreadMeta::getThreadId()%forwardersNcatchers;
	std::string socketAddr = idStr.str();
	zmq::message_t zmqmsgAddr(socketAddr.size());
	memcpy(zmqmsgAddr.data(), socketAddr.data(), socketAddr.size());

	int threadId = ThreadMeta::getThreadId();
	LOG_DEBUG("ZMQA :Message send to %s using socket %d\n", socketAddr.c_str(), threadId);
	zmq::socket_t* senderSocket = getThreadSocket();
	senderSocket->send(zmqmsgAddr, ZMQ_SNDMORE);
	senderSocket->send(zmqmsgBase);
}

void ZMQNetworkAsyncPoll::sendCallbackMessage(int toNodeId, HyflowMessage & message, HyflowMessageFuture & fu){
	// Serialize the message
	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << message;
	std::string msgData = ostream.str();

	zmq::message_t zmqmsg(msgData.size());
	memcpy(zmqmsg.data(), msgData.data(), msgData.size());

	// Add Address message part
	std::stringstream idStr;
	idStr<<"catcher-"<<toNodeId<<"-"<<ThreadMeta::getThreadId()%forwardersNcatchers;
	std::string socketAddr = idStr.str();
	zmq::message_t zmqmsgAddr(socketAddr.size());
	memcpy(zmqmsgAddr.data(), socketAddr.data(), socketAddr.size());

	int threadId = ThreadMeta::getThreadId();
	LOG_DEBUG("ZMQA :Message send to %s using socket %d\n", socketAddr.c_str(), threadId);
	zmq::socket_t* senderSocket = getThreadSocket();
	senderSocket->send(zmqmsgAddr, ZMQ_SNDMORE);
	senderSocket->send(zmqmsg);
	while (1){
		zmq::message_t zmqReply;
		senderSocket->recv(&zmqReply);
		LOG_DEBUG("ZMQA : Got send callback response\n");
	    int64_t more = 1;
	    size_t more_size = sizeof (more);
	    senderSocket->getsockopt(ZMQ_RCVMORE, &more, &more_size);
	    if (!more) {
			// Pass the last message part to callbackHandler
	    	LOG_DEBUG("ZMQA : Handling callback response\n");
			callbackHandler(zmqReply);
	        break;      //  Last message frame
	    }
	}
}

bool ZMQNetworkAsyncPoll::defaultHandler(zmq::message_t & msg) {
	LOG_DEBUG("ZMQA :Got the Network Event \n");
	if(msg.data() && (msg.size() > 0)) {
		// Read Message
		std::string idata(static_cast<char*>(msg.data()), msg.size());
		std::istringstream idata_stream(idata);
		boost::archive::text_iarchive ia(idata_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

//		if (req.msg_t != MSG_TYPE_INVALID)
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

void ZMQNetworkAsyncPoll::callbackHandler(zmq::message_t & msg){
	LOG_DEBUG("ZMQA :Got the Network Callback\n");
	if(msg.data() && (msg.size() > 0)) {
		std::string data(static_cast<char*>(msg.data()), msg.size());
		std::istringstream data_stream(data);
		boost::archive::text_iarchive ia(data_stream);
		vt_dstm::HyflowMessage req;
		ia >> req;

		LOG_DEBUG("ZMQA : Callback from node %d\n", req.toNode);
//		if (req.msg_t != MSG_TYPE_DUMMY)
		MessageHandler::callbackHandler(req);
	}
}

void* ZMQNetworkAsyncPoll::forwarderThread(void *param) {
	int forwarderId = *((int*)param);
	ThreadMeta::threadInit(-1, FORWARDER_THREAD);
	s_catch_signals();
	delete (int*)param;

	std::stringstream fromStr;
	fromStr<<"forwarder-"<<forwarderId;
	std::string fid=fromStr.str();
	std::string faddress = "inproc://"+fid;

	zmq::socket_t fromThreadSocket(*context, ZMQ_ROUTER);
	fromThreadSocket.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	fromThreadSocket.setsockopt(ZMQ_IDENTITY, fid.c_str(), fid.size());
	fromThreadSocket.bind(faddress.c_str());

	std::stringstream taddrStr;
	taddrStr<<"forwarder-"<<nodeId<<"-"<<forwarderId;
	std::string tid = taddrStr.str();
	std::stringstream toStr;
	toStr<<"tcp://"<<NetworkManager::getNodeIP()<<":"<<NetworkManager::getBasePort()+2*nodeId*(forwardersNcatchers)+forwarderId;
	std::string taddress=toStr.str();

	zmq::socket_t toNodesSocket(*context, ZMQ_ROUTER);
	toNodesSocket.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	toNodesSocket.setsockopt(ZMQ_IDENTITY, tid.c_str(), tid.size());
	toNodesSocket.bind(taddress.c_str());
	sleep(2);

	//Create dealSockets and poll set and wait
	zmq::pollitem_t nodeSet[2];
	nodeSet[0].socket = fromThreadSocket;
	nodeSet[0].fd = 0;
	nodeSet[0].events = ZMQ_POLLIN;
	nodeSet[0].revents = 0;

	nodeSet[1].socket = toNodesSocket;
	nodeSet[1].fd = 0;
	nodeSet[1].events = ZMQ_POLLIN;
	nodeSet[1].revents = 0;

	LOG_DEBUG("ZMQA forwarder %d started at %s and %s\n", forwarderId, faddress.c_str(), taddress.c_str());
	// Last thread synchronization
	if (forwarderId == (forwardersNcatchers-1)) {
	    boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
	    nodeReady = true;
	    LOG_DEBUG("ZMQA : Last forwarder Notifying the nodeReady\n");
	    nodeReadyCondition.notify_one();
	}


	while (!hyflowShutdown) {
		try {
			zmq::poll(nodeSet, 2, -1);
			if (nodeSet[0].revents & ZMQ_POLLIN) {
				// Got Message from threads to send to threads
				// We will get 2 frame one for sender, other for workLoad
				zmq::message_t senderMeta, toNodeMeta, workLoad;
				int64_t more = 1;
				size_t more_size = sizeof(more);
				fromThreadSocket.recv(&senderMeta);
				fromThreadSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				if (!more) {
					Logger::fatal("ZMQAP :Message from thread ended Unexpectedly\n");
				} else {
					LOG_DEBUG("ZMQAP :Forwarder received senderMeta\n");
					int64_t more1 = 1;
					fromThreadSocket.recv(&toNodeMeta);
					fromThreadSocket.getsockopt(ZMQ_RCVMORE, &more1, &more_size);
					if (!more1) {
						Logger::fatal("ZMQAP :Message from thread ended Unexpectedly2\n");
					} else {
						LOG_DEBUG("ZMQAP :Forwarder received toNodeMeta\n");
						int64_t more2 = 1;
						fromThreadSocket.recv(&workLoad);
						fromThreadSocket.getsockopt(ZMQ_RCVMORE, &more2, &more_size);
						if(more2) {
							Logger::fatal("ZMQAP :Unexpectedly long message from thread to dealer\n");
						}else {
							LOG_DEBUG("ZMQAP :Forwarder received workLoad\n");
							std::string toNode(static_cast<char*>(toNodeMeta.data()), toNodeMeta.size());
							std::string sender(static_cast<char*>(senderMeta.data()), senderMeta.size());
							std::string work(static_cast<char*>(workLoad.data()), workLoad.size());
							toNodesSocket.send(toNodeMeta, ZMQ_SNDMORE);
							toNodesSocket.send(senderMeta, ZMQ_SNDMORE);
							toNodesSocket.send(workLoad);
							LOG_DEBUG("ZMQAP :Forwarder send Message\n");
							LOG_DEBUG("ZMQAP :%s\n", toNode.c_str());
							LOG_DEBUG("ZMQAP :%s\n", sender.c_str());
							LOG_DEBUG("ZMQAP :%s\n", work.c_str());
						}
					}
				}
			}else {
				// We got reply from Nodes for threads
				zmq::message_t catcherMeta, threadMeta, workLoadResponse;
				int64_t more = 1;
				size_t more_size = sizeof(more);
				toNodesSocket.recv(&catcherMeta);
				toNodesSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				if (!more) {
					Logger::fatal("ZMQAP :Message from thread ended Unexpectedly\n");
				} else {
					int64_t more1 = 1;
					toNodesSocket.recv(&threadMeta);
					toNodesSocket.getsockopt(ZMQ_RCVMORE, &more1, &more_size);
					if (!more1) {
						Logger::fatal("ZMQAP :Message from thread ended Unexpectedly2\n");
					} else {
						int64_t more2 = 1;
						toNodesSocket.recv(&workLoadResponse);
						toNodesSocket.getsockopt(ZMQ_RCVMORE, &more2, &more_size);
						if(more2) {
							Logger::fatal("ZMQAP :Unexpectedly long message from thread to dealer\n");
						}else {
							fromThreadSocket.send(threadMeta, ZMQ_SNDMORE);
							fromThreadSocket.send(workLoadResponse);
							LOG_DEBUG("ZMQAP :Forwarder replied response Message\n");
						}
					}
				}

			}
		} catch(zmq::error_t & e) {
			if (hyflowShutdown) {
				LOG_DEBUG("ZMQAP :Forwarder %d exiting\n", forwarderId);
				break;
			}else {
				throw e;
			}
		}
	}

	return NULL;
}

void* ZMQNetworkAsyncPoll::catcherThread(void *param) {
	int catcherId = *((int*)param);
	ThreadMeta::threadInit(-1, CATCHER_THREAD);
	s_catch_signals();
	delete (int*)param;

	std::stringstream taddrStr;
	taddrStr<<"catcher-"<<nodeId<<"-"<<catcherId;
	std::string tid = taddrStr.str();
	std::stringstream toStr;
	toStr<<"tcp://"<<NetworkManager::getIp(nodeId)<<":"<<NetworkManager::getBasePort()+2*nodeId*(forwardersNcatchers)+(forwardersNcatchers)+catcherId;
	std::string taddress=toStr.str();
	zmq::socket_t fromNodesSocket(*context, ZMQ_ROUTER);
	fromNodesSocket.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	fromNodesSocket.setsockopt(ZMQ_IDENTITY, tid.c_str(), tid.size());
	fromNodesSocket.bind(taddress.c_str());

	std::stringstream fromStr;
	fromStr<<"catcherMessaging-"<<catcherId;
	std::string cmid=fromStr.str();
	std::string cmaddress = "inproc://"+cmid;
	zmq::socket_t catcherMessagingSocket(*context, ZMQ_ROUTER);
	catcherMessagingSocket.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	catcherMessagingSocket.setsockopt(ZMQ_IDENTITY, cmid.c_str(), cmid.size());
	catcherMessagingSocket.bind(cmaddress.c_str());

	std::stringstream wStr;
	wStr<<"catcherControl-"<<catcherId;
	std::string ccid=wStr.str();
	std::string ccaddress = "inproc://"+ccid;
	zmq::socket_t catcherControlSocket(*context, ZMQ_ROUTER);
	catcherControlSocket.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	catcherControlSocket.setsockopt(ZMQ_IDENTITY, ccid.c_str(), ccid.size());
	catcherControlSocket.bind(ccaddress.c_str());

	sleep(2);

	//Connect from Node socket to all nodes forwarders with same Id
	for(int i=0 ; i<nodeCount ; i++) {
		std::stringstream cStr;
		cStr<<"tcp://"<<nodeIPs[i]<<":"<<NetworkManager::getBasePort()+2*i*(forwardersNcatchers)+catcherId;
		std::string caddress=cStr.str();
		fromNodesSocket.connect(caddress.c_str());
		LOG_DEBUG("ZMQAP :Catcher %s connected to forwarder at %s\n", tid.c_str(), caddress.c_str());
		sleep(2);
	}


	//Create worker threads
	std::vector<pthread_t> workers;
	for( int i=0 ; i<catcherWorkers ; i++ ) {
		pthread_t worker;
		int* ids= new int[2];
		ids[0] = catcherId;
		ids[1] = i;
		pthread_create(&worker, NULL, ZMQNetworkAsyncPoll::workLoadProcessor, ids);
		workers.push_back(worker);
	}
	sleep(2);

	// Wait for all worker to show-up
	for( int i=0 ; i<catcherWorkers ; i++ ) {
		int64_t more = 1;
		size_t more_size = sizeof(more);
		zmq::message_t workerIdMsg1, startMessage;
		catcherMessagingSocket.recv(&workerIdMsg1);
		std::string workerIdMsgStr(static_cast<char*>(workerIdMsg1.data()), workerIdMsg1.size());
		LOG_DEBUG("ZMQAP :Catcher Received from worker %s\n", workerIdMsgStr.c_str());
		catcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
		if(!more) {
			Logger::fatal("ZMQAP : Received incomplete start message from worker\n");
		}else {
			catcherMessagingSocket.recv(&startMessage);
			std::string startMessageStr(static_cast<char*>(startMessage.data()), startMessage.size());
			int64_t more1 = 1;
			catcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more1, &more_size);
			LOG_DEBUG("ZMQAP :Worker %d got->%s\n", i, startMessageStr.c_str());
			if (more1) {
				Logger::fatal("ZMQAP : Received extra message from worker\n");
			}
		}
	}
	sleep(2);

	LOG_DEBUG("ZMQA Catcher %s started at %s, %s and %s\n", tid.c_str(), taddress.c_str(), cmaddress.c_str(), ccaddress.c_str());
	// Last thread synchronization
	if (catcherId == forwardersNcatchers -1) {
	    boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
	    nodeReady = true;
	    LOG_DEBUG("ZMQA : Last Catcher Notifying the nodeReady\n");
	    nodeReadyCondition.notify_one();
	}

	//Create dealSockets and poll set and wait
	zmq::pollitem_t nodeSet[2];
	nodeSet[0].socket = fromNodesSocket;
	nodeSet[0].fd = 0;
	nodeSet[0].events = ZMQ_POLLIN;
	nodeSet[0].revents = 0;

	nodeSet[1].socket = catcherMessagingSocket;
	nodeSet[1].fd = 0;
	nodeSet[1].events = ZMQ_POLLIN;
	nodeSet[1].revents = 0;

	while (!hyflowShutdown) {
		try {
			zmq::poll(nodeSet, 2, -1);
			if (nodeSet[0].revents & ZMQ_POLLIN) {
				// Got Message from Nodes to send to worker
				// We will get 2 frame one for sender, other for workLoad
				zmq::message_t senderMeta, fromForwarder, workLoad;
				int64_t more = 1;
				size_t more_size = sizeof(more);
				fromNodesSocket.recv(&fromForwarder);
				fromNodesSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				if (!more) {
					Logger::fatal("ZMQAP :Message from thread ended Unexpectedly\n");
				} else {
					int64_t more1 = 1;
					fromNodesSocket.recv(&senderMeta);
					fromNodesSocket.getsockopt(ZMQ_RCVMORE, &more1, &more_size);
					if (!more1) {
						Logger::fatal("ZMQAP :Message from thread ended Unexpectedly2\n");
					} else {
						int64_t more2 = 1;
						fromNodesSocket.recv(&workLoad);
						fromNodesSocket.getsockopt(ZMQ_RCVMORE, &more2, &more_size);
						if(more2) {
							Logger::fatal("ZMQAP :Unexpectedly long message from thread to dealer\n");
						}else {
							// This part will contain the dealer Id, which we don't want
							int64_t more3 = 1;
							zmq::message_t workerIdMeta, readyWorker;
							catcherControlSocket.recv(&workerIdMeta);
							catcherControlSocket.getsockopt(ZMQ_RCVMORE, &more3, &more_size);
							if(!more3) {
								Logger::fatal("ZMQAP : Received incomplete ready message from worker\n");
							}else {
								catcherControlSocket.recv(&readyWorker);
								int64_t more4 = 1;
								catcherControlSocket.getsockopt(ZMQ_RCVMORE, &more4, &more_size);
								if (more4) {
									Logger::fatal("ZMQAP : Received extra message from worker\n");
								}
							}
							std::string readyWorkerStr(static_cast<char*>(readyWorker.data()), readyWorker.size());
							catcherMessagingSocket.send(readyWorker, ZMQ_SNDMORE);
							catcherMessagingSocket.send(fromForwarder, ZMQ_SNDMORE);
							catcherMessagingSocket.send(senderMeta, ZMQ_SNDMORE);
							catcherMessagingSocket.send(workLoad);
							LOG_DEBUG("ZMQAP :Catcher sending Message to workerId-> %s\n", readyWorkerStr.c_str());
						}
					}
				}
			}else {
				// We got reply from workers for Nodes
				zmq::message_t senderMeta, fromForwarder, workLoadResponse, workerIdMeta;
				int64_t more = 1;
				size_t more_size = sizeof(more);
				catcherMessagingSocket.recv(&workerIdMeta);
				catcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				if (!more) {
					Logger::fatal("ZMQAP :Message from thread ended Unexpectedly\n");
				} else {
					int64_t more1 = 1;
					catcherMessagingSocket.recv(&fromForwarder);
					catcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more1, &more_size);
					if (!more1) {
						Logger::fatal("ZMQAP :Message from thread ended Unexpectedly2\n");
					} else {
						int64_t more2 = 1;
						catcherMessagingSocket.recv(&senderMeta);
						catcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more2, &more_size);
						if(!more2) {
							Logger::fatal("ZMQAP :Message from thread ended Unexpectedly3\n");
						}else {
							int64_t more3 = 1;
							catcherMessagingSocket.recv(&workLoadResponse);
							catcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more3, &more_size);
							if(more3) {
								Logger::fatal("ZMQAP :Unexpectedly long message from thread to dealer\n");
							}else {
								fromNodesSocket.send(fromForwarder, ZMQ_SNDMORE);
								fromNodesSocket.send(senderMeta, ZMQ_SNDMORE);
								fromNodesSocket.send(workLoadResponse);
								LOG_DEBUG("ZMQAP :Catcher replied Message\n");
							}
						}
					}
				}
			}
		} catch(zmq::error_t & e) {
			if (hyflowShutdown) {
				for( int i=0 ; i<catcherWorkers ; i++ ) {
					pthread_t worker = workers.at(i);
					pthread_kill(worker, SIGINT);
					pthread_join(worker, 0);
				}
				LOG_DEBUG("ZMQAP :Catcher %d exiting\n", catcherId);
				break;
			}else {
				throw e;
			}
		}
	}
	return NULL;
}

void* ZMQNetworkAsyncPoll::workLoadProcessor(void *param) {
	int *ids = (int*) param;
	int catcherId = ids[0];
	int workerId = catcherId*catcherWorkers+ids[1];
	ThreadMeta::threadInit(workerId, WORKER_THREAD);
	s_catch_signals();
	delete[] ids;

	std::stringstream wlAddrStr;
	wlAddrStr<<"catcherMessaging-"<<catcherId;
	std::string wlAddrId = wlAddrStr.str();
	std::string wlAddr = "inproc://"+wlAddrId;
	std::stringstream wlIdStr;
	wlIdStr<<"workerMessaging-"<<workerId;
	std::string wlId = wlIdStr.str();

	std::stringstream wcAddrStr;
	wcAddrStr<<"catcherControl-"<<catcherId;
	std::string wcAddrId = wcAddrStr.str();
	std::string wcAddr = "inproc://"+wcAddrId;
	std::stringstream wcIdStr;
	wcIdStr<<"workerControl-"<<workerId;
	std::string wcId = wcIdStr.str();

	zmq::socket_t forCatcherMessagingSocket(*context, ZMQ_DEALER);
	forCatcherMessagingSocket.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	forCatcherMessagingSocket.setsockopt(ZMQ_IDENTITY, wlId.c_str(), wlId.size());
	forCatcherMessagingSocket.connect(wlAddr.c_str());

	zmq::socket_t forCatcherControlSender(*context, ZMQ_DEALER);
	forCatcherControlSender.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	forCatcherControlSender.setsockopt(ZMQ_IDENTITY, wcId.c_str(), wcId.size());
	forCatcherControlSender.connect(wcAddr.c_str());

	LOG_DEBUG("ZMQA : WorkLoad Processor %s started at %s and %s\n", wlId.c_str(), wlAddr.c_str(), wcAddr.c_str());
	sleep(2);

	// Tell catcher worker ready
	std::string readyStr("Ready");
	zmq::message_t readyMsg(readyStr.size());
	memcpy(readyMsg.data(), readyStr.data(), readyStr.size());
	forCatcherMessagingSocket.send(readyMsg);
	LOG_DEBUG("ZMQAP :Worker send message to catcher %s\n", wcAddrId.c_str());

	while(!hyflowShutdown) {
		try {
			LOG_DEBUG("ZMQA :Worker Asking for work by sending its load socket Id\n");
			zmq::message_t workerMessage(wlId.size());
			memcpy(workerMessage.data(), wlId.data(), wlId.size());
			//Send Request to WorkDistributer requesting work
			forCatcherControlSender.send(workerMessage);

			//Wait for workLoad from Catcher
			zmq::message_t forwarderMeta, senderMeta, workLoad;
			forCatcherMessagingSocket.recv(&forwarderMeta);
			LOG_DEBUG("ZMQA :Worker received forwarder Address\n");
			int64_t more = 1;
			size_t more_size = sizeof(more);
			forCatcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
			if (!more) {
				Logger::fatal("ZMQA : Worker Message part 1 ended unexpectedly\n");
			}else {
				forCatcherMessagingSocket.recv(&senderMeta);
				LOG_DEBUG("ZMQA :Worker received SenderMeta data\n");
				int64_t more = 1;
				size_t more_size = sizeof(more);
				forCatcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				if (!more) {
					Logger::fatal("ZMQA :Message part 2 ended unexpectedly\n");
				}else {
					//  Last message frame
					forCatcherMessagingSocket.recv(&workLoad);
					LOG_DEBUG("ZMQA :Worker received WorkLoad\n");
					int64_t more = 1;
					size_t more_size = sizeof(more);
					forCatcherMessagingSocket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
					if (more) {
						Logger::fatal("ZMQA : Worker Message part 3 unexpectedly long\n");
					}
					if (defaultHandler(workLoad)) {
						LOG_DEBUG("ZMQA : Worker sending Callback Message Response\n");
						forCatcherMessagingSocket.send(forwarderMeta, ZMQ_SNDMORE);
						forCatcherMessagingSocket.send(senderMeta, ZMQ_SNDMORE);
						forCatcherMessagingSocket.send(workLoad);
					}
				}
			}
		}catch(zmq::error_t & e) {
			if (hyflowShutdown){
				LOG_DEBUG("ZMQA : Worker Processor %d exiting\n", catcherId);
				break;
			}else {
				throw e;
			}
		}
	}
	return NULL;
}

void ZMQNetworkAsyncPoll::s_catch_signals() {
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

void ZMQNetworkAsyncPoll::s_signal_handler(int signal_value) {
	LOG_DEBUG("ZMQA :Server Thread Signalled!!\n");
}

} /* namespace vt_dstm */
