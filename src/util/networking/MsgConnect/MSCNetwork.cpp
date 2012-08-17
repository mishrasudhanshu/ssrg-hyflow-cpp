/*
 * MCNetwork.cpp
 *
 *  Created on: Aug 16, 2012
 *      Author: mishras[at]vt.edu
 */

#include <stdio.h>

#include "MSCNetwork.h"
#include "MSCMessage.h"

namespace vt_dstm
{
MSCNetwork* MSCNetwork::instance = NULL;

MSCNetwork::MSCNetwork() {
	if (!instance) {
		instance = new MSCNetwork();
		instance->messenger = new MsgConnect::MCMessenger();
		instance->queue = new MsgConnect::MCQueue();
		instance->socket = new MsgConnect::MCSocketTransport();
		instance->handlers = queue->getHandlers();
	}
}

MSCNetwork::~MSCNetwork() {
	delete instance->messenger;
	delete instance->queue;
	delete instance->socket;
}

//FIXME: create NetworkDeinit
void MSCNetwork::NetworkInit(){
	instance->messenger->setMaxTimeout(ULONG_MAX);
	instance->socket->setActive(false);
	instance->socket->setAttemptsToConnect(1);
	instance->socket->setFailOnInactive(true);
	instance->socket->setMaxTimeout(900000l);
	// FIXME: Update IP and Port
	instance->socket->setMessengerAddress("127.0.0.1");
	instance->socket->setMessengerPort(14583);
	instance->socket->setTransportMode(stmP2P);
	instance->socket->setMessenger(messenger);
	instance->socket->setActive(true);

	instance->queue->setQueueName("Queue");
	instance->queue->setMessenger(messenger);

	MsgConnect::MCMessageHandler* handler = instance->handlers->Add();
	handler->setMsgCodeLow(1);
	handler->setMsgCodeHigh(1);
	handler->setOnMessage(MSCNetwork::defaultHandler);
	handler->setEnabled(true);
}

void MSCNetwork::sendMessage(int nodeId, AbstractMessage Message){

}

AbstractMessage MSCNetwork::sendCallbackMessage(int nodeId, AbstractMessage Message){
	MSCMessage *msg = NULL ;
	return *msg;
}

void MSCNetwork::registerHandler(MessageType msg_t, void (*handlerFunc)(AbstractMessage)){
	instance->handlerMap[msg_t] = handlerFunc;
}

void MSCNetwork::defaultHandler(void* UserData, void* Sender,
	MsgConnect::MCMessage& Message, bool& Handled) {

}

}

