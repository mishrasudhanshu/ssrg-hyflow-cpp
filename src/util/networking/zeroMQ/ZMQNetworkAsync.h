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
#include "../AbstractNetwork.h"

namespace vt_dstm {

class ZMQNetworkAsync: public vt_dstm::AbstractNetwork {
	static int nodeId;
	static int basePort;
	static int threadCount;
	static int nodeCount;

	static volatile bool hyflowShutdown;
	static int lingerTime;

	zmq::context_t* context;
	/*
	 * Router Socket provided to each thread to route its message to desired
	 * node
	 */
	static std::vector<zmq::socket_t*> threadRouterSockets;
	/*
	 * Dealer sockets which wait for incoming messages to process.
	 */
	static std::vector<zmq::socket_t*> recieverDealerSockets;

	static std::vector<pthread_t> dealerThreads;
	static std::vector<int*> dealerThreadIds;

	static bool isInit;

	/*
	 * Tells the default Handler whether send message was a callback or not
	 */
	static bool defaultHandler(zmq::message_t & msg);
	static void callbackHandler(zmq::message_t & msg);
public:
	ZMQNetworkAsync();
	virtual ~ZMQNetworkAsync();

	void networkInit();
	void networkShutdown();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
	static void* dealerExecute(void *param);
	static void connectClient(int nodeId);
	static void additionalSync();

	static void s_catch_signals();
	static void s_signal_handler(int signal_value);
};

} /* namespace vt_dstm */

#endif /* ZMQNETWORKASYNC_H_ */
