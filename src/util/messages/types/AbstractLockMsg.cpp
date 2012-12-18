/*
 * AbstractLockMsg.cpp
 *
 *  Created on: Dec 15, 2012
 *      Author: mishras[at]vt.edu
 */

#include "AbstractLockMsg.h"
#include "../../logging/Logger.h"
#include "../../../core/context/AbstractLockTable.h"

#include "../../networking/NetworkManager.h"
#include "../../messages/MessageMaps.h"
#include "../HyflowMessageFuture.h"

namespace vt_dstm {

AbstractLockMsg::AbstractLockMsg(AbstractLock* absLock, unsigned long long tid, bool lock) {
	txnId = tid;
	doLock = lock;
	request = true;
	abstractLock = absLock;
}

AbstractLockMsg::~AbstractLockMsg() {}

void AbstractLockMsg::absLockAccessHandler(HyflowMessage& m) {
	AbstractLockMsg *almsg = (AbstractLockMsg*) (m.getMsg());
	if (almsg->request) {
		if (almsg->doLock) {
			bool resp = AbstractLockTable::tryLock(almsg->abstractLock->getLockName(), *(almsg->abstractLock), almsg->read);
			LOG_DEBUG("ABL :Got Lock  request for %s\n", almsg->abstractLock->getLockName().c_str());
			almsg->response = resp;
		}else {
			AbstractLockTable::unlock(almsg->abstractLock->getLockName());
		}
		/* If message as callback and network library don't support the callback, we need to reply manually*/
		if (m.isCallback) {
			if (!m.isCallbackSupported) {
				NetworkManager::sendMessage(m.fromNode, m);
			}
		}
	}else {
		if (almsg->doLock) {
			// Find the MessageFuture created for expected response
			HyflowMessageFuture* cbfmsg = MessageMaps::getMessageFuture(m.msg_id,
					m.msg_t);
			if (cbfmsg) {
				cbfmsg->setBoolResponse(almsg->response);
			}else {
				Logger::fatal("Can not find Object access future for m_id %s\n", m.msg_id.c_str());
			}
		}
		// Unlock response can be ignored
	}
}

} /* namespace vt_dstm */
