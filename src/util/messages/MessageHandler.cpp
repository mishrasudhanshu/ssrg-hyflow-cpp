/*
 * MessageHandler.cpp
 *
 *  Created on: Sep 18, 2012
 *      Author: mishras[at]vt.edu
 */

#include "MessageHandler.h"
#include "MessageMaps.h"
#include "../logging/Logger.h"

#include "../../core/context/ContextManager.h"

namespace vt_dstm {

MessageHandler::MessageHandler() {}

MessageHandler::~MessageHandler() {}

void MessageHandler::msgHandler(HyflowMessage & req) {
	// Handle Message
	req.syncClocks();
	void (*handler)(HyflowMessage &) = NULL;
	handler = MessageMaps::getMessageHandler(req.msg_t);
	if ( handler != NULL)
		handler(req);
	else {
		Logger::fatal("Message Handler Not available \n");
	}

	// Update from node clock
	req.fromNodeClock = ContextManager::getClock();
}

void MessageHandler::callbackHandler(HyflowMessage & req) {
		req.syncClocks();

		void (*handler)(HyflowMessage &) = NULL;
		handler = MessageMaps::getMessageHandler(req.msg_t);
		if ( handler != NULL)
			handler(req);
		else {
			Logger::fatal("Message Handler Not available \n");
		}
}

void MessageHandler::registerMessageFuture(const std::string & m_id, HyMessageType t, HyflowMessageFuture & fu) {
	MessageMaps::registerMessageFuture(m_id, t, fu);
}

} /* namespace vt_dstm */
