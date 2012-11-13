/*
 * ZMQNetworkAsyncPoll.h
 *
 *  Created on: Nov 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ZMQNETWORKASYNCPOLL_H_
#define ZMQNETWORKASYNCPOLL_H_

#include <vector>
#include <pthread.h>
#include "zmq.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/tss.hpp"
#include "../AbstractNetwork.h"

namespace vt_dstm {

class ZMQNetworkAsyncPoll: public vt_dstm::AbstractNetwork {
	static int nodeId;
	static int basePort;
	static int threadCount;
	static int nodeCount;
	static int zeroMQTFR;
	static int zeroMQWFR;
	static int forwardersNcatchers;
	static int catcherWorkers;

	static volatile bool hyflowShutdown;
	static volatile bool nodeReady;
	static boost::condition nodeReadyCondition;
	static boost::mutex nodeReadyMutex;

	static int lingerTime;
	static std::string* nodeIPs;

	static zmq::context_t* context;
	static boost::thread_specific_ptr<zmq::socket_t> thread_socket;
	/*
	 * Router Socket provided to each thread to route its message to desired
	 * node, we save a pointer to so that we can clean up at exit time
	 */
	static std::vector<zmq::socket_t*> threadRouterSockets;

	static zmq::socket_t* nodeInitSocket;
	static zmq::socket_t* mainThreadSocket;

	static std::vector<pthread_t> forwarderThreads;
	static std::vector<pthread_t> catcherThreads;
	static std::vector<int*> forwarderThreadIds;
	static std::vector<int*> catcherThreadIds;
	static pthread_t ForwarderThread;

	static bool isInit;

	/*
	 * Tells the default Handler whether send message was a callback or not
	 */
	static bool defaultHandler(zmq::message_t & msg);
	static void callbackHandler(zmq::message_t & msg);
	static void* forwarderThread(void *param);
	static void* catcherThread(void *param);
	static void* workLoadProcessor(void *param);
	static void additionalSync();

	static void s_catch_signals();
	static void s_signal_handler(int signal_value);
	static zmq::socket_t* getThreadSocket();
public:
	ZMQNetworkAsyncPoll();
	virtual ~ZMQNetworkAsyncPoll();

	void networkInit();
	void networkShutdown();
	void threadNetworkInit();
	void threadNetworkShutdown();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
};

} /* namespace vt_dstm */

#endif /* ZMQNETWORKASYNCPOLL_H_ */
