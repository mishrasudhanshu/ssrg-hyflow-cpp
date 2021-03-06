/*
 * ZMQNetworkAsync.h
 *
 *  Created on: Oct 5, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ZMQNETWORKASYNC_H_
#define ZMQNETWORKASYNC_H_

#include <vector>
#include <pthread.h>
#include "zmq.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include "../AbstractNetwork.h"

namespace vt_dstm {

class ZMQNetworkAsync: public vt_dstm::AbstractNetwork {
	static int nodeId;
	static int basePort;
	static int threadCount;
	static int nodeCount;

	static volatile bool hyflowShutdown;
	static volatile bool nodeReady;
	static boost::condition nodeReadyCondition;
	static boost::mutex nodeReadyMutex;

	static int lingerTime;
	static std::string* nodeIPs;

	static zmq::context_t* context;
	/*
	 * Router Socket provided to each thread to route its message to desired
	 * node
	 */
	static std::vector<zmq::socket_t*> threadRouterSockets;

	static zmq::socket_t* nodeInitSocket;

	static std::vector<pthread_t> dealerThreads;
	static std::vector<pthread_t> workerThreads;
	static std::vector<int*> dealerThreadIds;
	static std::vector<int*> workerThreadIds;
	static pthread_t distributerThread;

	static bool isInit;

	/*
	 * Tells the default Handler whether send message was a callback or not
	 */
	static bool defaultHandler(zmq::message_t & msg);
	static void callbackHandler(zmq::message_t & msg);
	static void* dealerExecute(void *param);
	static void additionalSync();
	static void* workLoadDistributer(void *param);
	static void* workLoadProcessor(void *param);

	static void s_catch_signals();
	static void s_signal_handler(int signal_value);
public:
	ZMQNetworkAsync();
	virtual ~ZMQNetworkAsync();

	void networkInit();
	void networkShutdown();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
};

} /* namespace vt_dstm */

#endif /* ZMQNETWORKASYNC_H_ */
