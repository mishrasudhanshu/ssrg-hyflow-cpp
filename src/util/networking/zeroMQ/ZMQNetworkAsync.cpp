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

#define WORKER_STR "worker"

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
std::vector<pthread_t> ZMQNetworkAsync::workerThreads;
pthread_t ZMQNetworkAsync::distributerThread;
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


		// Create workDistributer thread for this node
		pthread_create(&distributerThread,NULL,ZMQNetworkAsync::workLoadDistributer, NULL);
		sleep(4);	// Make sure distributor is initiated
		// Create workProcessor Threads for this node
		for(int workerCount=0 ; workerCount < threadCount ; workerCount++){
			pthread_t workerThread;
			pthread_create(&workerThread, NULL, workLoadProcessor, (void*)workerCount);
			workerThreads.push_back(workerThread);
		}

		// Wait for last worker thread to signal back
		{
			boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
			while ( !nodeReady )
				nodeReadyCondition.wait(nodeReadyMutex);
		}
		nodeReady = false;
		sleep(2);


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
	LOG_DEBUG("ZMQA : Dealer Executor threads killed\n");

	for (unsigned int workerCount=0 ; workerCount < workerThreads.size(); workerCount++) {
		pthread_kill(workerThreads[workerCount],SIGINT);
		pthread_join(workerThreads[workerCount], NULL);
	}
	LOG_DEBUG("ZMQA : Dealer Worker threads killed\n");

	sleep(2);
	pthread_kill(distributerThread, SIGINT);
	pthread_join(distributerThread, NULL);
	LOG_DEBUG("ZMQA : Distributor thread killed\n");

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
	zmq::pollitem_t* nodeSet = new zmq::pollitem_t[nodeCount+1];
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

	// Create socket to talk with WorkLoad Distributor
	std::stringstream distStr;
	distStr<<waitThread<<"-distro";
	std::string waiterId = distStr.str();
	zmq::socket_t distTalker(*context, ZMQ_DEALER);
	distTalker.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	distTalker.setsockopt(ZMQ_IDENTITY, waiterId.c_str(), waiterId.size());
	distTalker.connect("inproc://workLoadReceiver");

	nodeSet[nodeCount].socket = distTalker;
	nodeSet[nodeCount].fd = 0;
	nodeSet[nodeCount].events = ZMQ_POLLIN;
	nodeSet[nodeCount].revents = 0;

	sleep(2);

	// Last thread synchronization
	if (waitThread == threadCount-1) {
	    boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
	    nodeReady = true;
	    LOG_DEBUG("ZMQA : Last Dealer Notifying the nodeReady\n");
	    nodeReadyCondition.notify_one();
	}

	while (!hyflowShutdown) {
		try {
			zmq::poll(nodeSet, nodeCount+1, -1);
			for (int i = 0 ; i<= nodeCount ; i++) {
				if (nodeSet[i].revents & ZMQ_POLLIN) {
					zmq::socket_t *socket = dealerSockets[i];
					if ( i == nodeCount) {
						zmq::message_t metaMsg, workLoadResp;
						distTalker.recv(&metaMsg);
						int64_t more = 1;
						size_t more_size = sizeof (more);
						distTalker.getsockopt(ZMQ_RCVMORE, &more, &more_size);
						if (!more) {
							Logger::fatal("ZMQA : Message from distTalker ended Unexpectedly\n");
						}else {
							distTalker.recv(&workLoadResp);
							int64_t more1 = 1;
							size_t more1_size = sizeof (more1);
							distTalker.getsockopt(ZMQ_RCVMORE, &more1, &more1_size);
							if (more1) {
								Logger::fatal("ZMQA : Unexpectedly long message from distTalker to dealer\n");
							}else {
								// Reply back to requester
								std::string requesterIndex(static_cast<char*>(metaMsg.data()), metaMsg.size());
								int requester = atoi(requesterIndex.c_str());
								zmq::socket_t* requestSocket = dealerSockets[requester];
								requestSocket->send(workLoadResp);
								LOG_DEBUG("ZMQA : DealExecutor replied callback Message to %d\n", requester);
							}
						}
					}else {
						while (1) {		// MultiPart message checking not required
							zmq::message_t request;
							socket->recv(&request);
							LOG_DEBUG("ZMQA :Dealer received Message\n");
							int64_t more = 1;
							size_t more_size = sizeof (more);
							socket->getsockopt(ZMQ_RCVMORE, &more, &more_size);
							if (!more) {
								 //  Last message frame, create msgMetaData
								std::stringstream metaStr;
								metaStr<<i;
								std::string metaData = metaStr.str();
								zmq::message_t metaMsg(metaData.size());
								memcpy(metaMsg.data(), metaData.data(), metaData.size());

								//Forward received work load to distributor
								distTalker.send(metaMsg, ZMQ_SNDMORE);
								distTalker.send(request);
								LOG_DEBUG("ZMQA : Dealer Executor send WorkLoad Message to distributor\n");
								break;
							}
						}
					}
					//Check on other sockets.
				}
			}
		} catch(zmq::error_t & e) {
			if (hyflowShutdown) {
				LOG_DEBUG("ZMQA : Dealer Executor %d exiting\n", waitThread);
				break;
			}else {
				throw e;
			}
		}
	}

	for (unsigned int i = 0; i < dealerSockets.size();i++) {
		zmq::socket_t* saveSocket = dealerSockets[i];
		 dealerSockets[i] = NULL;
		delete saveSocket;
	}

	return NULL;
}

void* ZMQNetworkAsync::workLoadDistributer(void *param) {
	ThreadMeta::threadInit(0, DISPATCHER_THREAD);
	s_catch_signals();

	// Create front end Router socket which talk to all dealer Executors
	zmq::socket_t workLoadReceiver(*context, ZMQ_ROUTER);
	workLoadReceiver.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	workLoadReceiver.bind("inproc://workLoadReceiver");

	// Create back end Router socket which will distribute the work to available worker
	zmq::socket_t workLoadSender(*context, ZMQ_ROUTER);
	workLoadSender.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	workLoadSender.bind("inproc://workLoadSender");

	sleep(2);
	LOG_DEBUG("ZMQA : workLoadDistributer created and initiated\n");
	while (!hyflowShutdown) {
		try {
			// Wait for any incoming workload or workLoad Response
			int part = 1;
			bool isRequest=true;
			zmq::message_t inProcSender, workLoadMeta, workLoad, requestSender;
			while (1) { // MultiPart message checking
				if (part == 1) {
					// This part will contain the dealer Id, which we don't want
					isRequest = true;
					LOG_DEBUG("Distributor is waiting\n");
					workLoadReceiver.recv(&inProcSender);
					LOG_DEBUG("ZMQA :Distributor received in process Sender Address\n");
					int64_t more = 1;
					size_t more_size = sizeof(more);
					std::string senderName(static_cast<char*>(inProcSender.data()), inProcSender.size());
					if (senderName.substr(0,6).compare(WORKER_STR) == 0) {
						LOG_DEBUG("ZMQA :Distributor Message Type: workLoad response from %s\n", senderName.c_str());
						isRequest = false;
					}
					workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
					if (!more) {
						Logger::fatal("ZMQA :Distributor part 1 Message: %s ended unexpectedly\n", senderName.c_str());
					}else {
						part++;
					}
				}else if (part == 2) {
					if (isRequest) {
						workLoadReceiver.recv(&workLoadMeta);
						LOG_DEBUG("ZMQA :Distributor received SenderMeta data\n");
						int64_t more = 1;
						size_t more_size = sizeof(more);
						workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
						if (!more) {
							Logger::fatal("ZMQA :Distributor Message from dealer ended unexpectedly\n");
						}
						part++;
						continue;
					}else {
						workLoadReceiver.recv(&requestSender);
						LOG_DEBUG("ZMQA :Distributor received request sender thread Address\n");
						int64_t more = 1;
						size_t more_size = sizeof(more);
						workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
						if (!more) {
							Logger::fatal("ZMQA :Distributor part 2 Message from worker ended unexpectedly\n");
						}
						part++;
						continue;
					}
				}else if (part == 3) {
					if (isRequest) {
							//  Last message frame
							workLoadReceiver.recv(&workLoad);
							LOG_DEBUG("ZMQA :Distributor received WorkLoad\n");
							int64_t more = 1;
							size_t more_size = sizeof(more);
							workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
							if (more) {
								Logger::fatal("ZMQA :Distributor Message part 3 from dealer unexpectedly long\n");
							}
							part=1;
							break;
					} else {
						//  Actual sender Meta
						workLoadReceiver.recv(&workLoadMeta);
						LOG_DEBUG("ZMQA :Distributor received WorkLoad requester sender MetaData\n");
						int64_t more = 1;
						size_t more_size = sizeof(more);
						workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
						if (!more) {
							Logger::fatal("ZMQA :Distributor Message part 3 from worker 2 ended unexpectedly\n");
						}
						part++;
					}
				}else if (part ==4 ) {
					if (isRequest) {
						Logger::fatal("ZMQA : BUG - control should not reach here\n");
					}else {
						//  Last message frame, work load Response
						workLoadReceiver.recv(&workLoad);
						LOG_DEBUG("ZMQA :Distributor received WorkLoad Response\n");
						int64_t more = 1;
						size_t more_size = sizeof(more);
						workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
						if (more) {
							Logger::fatal("ZMQA :Message from worker unexpectedly long\n");
						}
						part = 1;
						break;
					}
				}
			}

			if (isRequest) {
				// Wait for any work thread to show up for requesting workload
				zmq::message_t availableWorkerAddr, workerMsg;
				int64_t more = 1;
				size_t more_size = sizeof(more);
				workLoadSender.recv(&availableWorkerAddr);
				workLoadSender.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				if (!more) {
					Logger::fatal("ZMQA :Message ended unexpectedly\n");
				}else {
					workLoadSender.recv(&workerMsg);
					int64_t more1 = 1;
					size_t more1_size = sizeof(more1);
					workLoadSender.getsockopt(ZMQ_RCVMORE, &more1, &more1_size);
					if (more1) {
						Logger::fatal("ZMQA :Message unexpectedly long\n");
					}
				}
				workLoadSender.send(availableWorkerAddr, ZMQ_SNDMORE);
				workLoadSender.send(inProcSender, ZMQ_SNDMORE);
				workLoadSender.send(workLoadMeta, ZMQ_SNDMORE);
				workLoadSender.send(workLoad);
				LOG_DEBUG("ZMQA : WorkLoad Request given to worker\n");
			} else {
				// Some workload got process lets send it back
				workLoadReceiver.send(requestSender, ZMQ_SNDMORE);
				workLoadReceiver.send(workLoadMeta, ZMQ_SNDMORE);
				workLoadReceiver.send(workLoad);
				LOG_DEBUG("ZMQA :Distributor workLoad Response send to requester\n");
			}
		}catch(zmq::error_t & e) {
			if (hyflowShutdown){
				LOG_DEBUG("ZMQA : WorkLoad Distributor Exiting\n");
				break;
			}else {
				throw e;
			}
		}
	}

	return NULL;
}

void* ZMQNetworkAsync::workLoadProcessor(void *param) {
	ThreadMeta::threadInit(0, DISPATCHER_THREAD);
	s_catch_signals();

	int workerId = (int)param;
	std::stringstream idStr;
	idStr<<WORKER_STR<<"-"<<workerId;
	std::string id = idStr.str();

	zmq::socket_t workLoadReceiver(*context, ZMQ_DEALER);
	workLoadReceiver.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	workLoadReceiver.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
	workLoadReceiver.connect("inproc://workLoadSender");

	zmq::socket_t workLoadSender(*context, ZMQ_DEALER);
	workLoadSender.setsockopt(ZMQ_LINGER, &lingerTime, sizeof lingerTime);
	workLoadSender.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
	workLoadSender.connect("inproc://workLoadReceiver");

	sleep(2);
	LOG_DEBUG("ZMQA : WorkLoad Processor %d started\n", workerId);

	// Last thread synchronization
	if (workerId == threadCount-1) {
		boost::unique_lock<boost::mutex> lock(nodeReadyMutex);
		nodeReady = true;
		LOG_DEBUG("ZMQA : Last Worker Notifying the nodeReady\n");
		nodeReadyCondition.notify_one();
	}

	while(!hyflowShutdown) {
		try {
			LOG_DEBUG("ZMQA :Worker Asking for work\n");
			std::string workerString("Give Me some Work");
			zmq::message_t workerMessage(workerString.size());
			memcpy(workerMessage.data(), workerString.data(), workerString.size());
			//Send Request to WorkDistributer requesting work
			workLoadReceiver.send(workerMessage);

			//Wait for workLoad from Distributor
			int part=1;
			zmq::message_t inProcSender, workLoadMeta, workLoad;
			while (1) { // MultiPart message checking
				if (part == 1) {
					// This part will contain the dealer Id, which we don't want
					workLoadReceiver.recv(&inProcSender);
					LOG_DEBUG("ZMQA :Worker received in process Sender Address\n");
					int64_t more = 1;
					size_t more_size = sizeof(more);
					workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
					if (!more) {
						Logger::fatal("ZMQA : Worker Message part 1 ended unexpectedly\n");
					}
					part++;
				}else if (part == 2) {
					workLoadReceiver.recv(&workLoadMeta);
					LOG_DEBUG("ZMQA :Worker received SenderMeta data\n");
					int64_t more = 1;
					size_t more_size = sizeof(more);
					workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
					if (!more) {
						Logger::fatal("ZMQA :Message part 2 ended unexpectedly\n");
					}
					part++;
				}else if (part == 3) {
					//  Last message frame
					workLoadReceiver.recv(&workLoad);
					LOG_DEBUG("ZMQA :Worker received WorkLoad\n");
					int64_t more = 1;
					size_t more_size = sizeof(more);
					workLoadReceiver.getsockopt(ZMQ_RCVMORE, &more, &more_size);
					if (more) {
						Logger::fatal("ZMQA : Worker Message part 3 unexpectedly long\n");
					}
					if (defaultHandler(workLoad)) {
						LOG_DEBUG("ZMQA : Worker sending Callback Message Response\n");
						workLoadSender.send(inProcSender, ZMQ_SNDMORE);
						workLoadSender.send(workLoadMeta, ZMQ_SNDMORE);
						workLoadSender.send(workLoad);
					}
					part=1;
					break;
				}
			}
		}catch(zmq::error_t & e) {
			if (hyflowShutdown){
				LOG_DEBUG("ZMQA : Worker Processor %d exiting\n", workerId);
				break;
			}else {
				throw e;
			}
		}
	}

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
