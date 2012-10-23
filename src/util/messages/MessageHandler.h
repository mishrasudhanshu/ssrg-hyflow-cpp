/*
 * MessageHandler.h
 * This class is created to separate the Network library definitions with
 * hyflow related functionalities and remove definition collisions.
 *  Created on: Sep 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include "HyflowMessage.h"
#include "HyflowMessageFuture.h"

namespace vt_dstm {

class MessageHandler {
public:
	MessageHandler();
	virtual ~MessageHandler();

	static void msgHandler(HyflowMessage & msg);
	static void callbackHandler(HyflowMessage & msg);

	static void registerMessageFuture(const std::string & m_id, HyMessageType t, HyflowMessageFuture & fu);
};

} /* namespace vt_dstm */

#endif /* MESSAGEHANDLER_H_ */
