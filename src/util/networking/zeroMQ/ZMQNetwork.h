/*
 * ZMQNetwork.h
 *
 *  Created on: Sep 29, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ZMQNETWORK_H_
#define ZMQNETWORK_H_

#include <vector>
#include "zmq.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "../AbstractNetwork.h"

namespace vt_dstm {

class ZMQNetwork: public AbstractNetwork {
	static int nodeId;
	static int basePort;
	static int threadCount;
	static int nodeCount;

	static volatile bool hyflowShutdown;

	zmq::context_t* context;
	static std::vector<boost::mutex*> socketMutexs;
	static std::vector<zmq::socket_t*> clientSockets;
	static zmq::socket_t * serverSocket;
	static boost::thread *serverThread;

	static bool isInit;

	/*
	 * Tells the default Handler whether send message was a callback or not
	 */
	static bool defaultHandler(zmq::message_t & msg);
	static void callbackHandler(zmq::message_t & msg);
public:
	ZMQNetwork();
	virtual ~ZMQNetwork();

	void networkInit();
	void sendMessage(int nodeId, HyflowMessage & Message);
	void sendCallbackMessage(int nodeId, HyflowMessage & Message, HyflowMessageFuture & fu);
	static void serverExecute();
	static void connectClient(int nodeId);
};

} /* namespace vt_dstm */

#endif /* ZMQNETWORK_H_ */
