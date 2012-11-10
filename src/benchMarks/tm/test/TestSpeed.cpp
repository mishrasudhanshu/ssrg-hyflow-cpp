/*
 * TestSpeed.cpp
 *
 *  Created on: Nov 10, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TestSpeed.h"
#include "../../../util/messages/types/DummyTestMsg.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include <string>

namespace vt_dstm {

TestSpeed::TestSpeed() {}

TestSpeed::~TestSpeed() {}

int TestSpeed::getOperandsCount() {
	return 0;
}
void TestSpeed::readOperation(std::string ids[], int size) {
	// Just send dummy messages
	int nodeCount = NetworkManager::getNodeCount();
	for (int i=0 ; i < 6 ; i++) {
		HyflowMessageFuture mFu;
		std::string id("0-0");
		HyflowMessage hmsg(id);
		hmsg.msg_t = MSG_TYPE_DUMMY;
		DummyTestMsg dmsg("Hi there");
		hmsg.setMsg(&dmsg);
		int toNode = i%nodeCount;
		NetworkManager::sendCallbackMessage(toNode, hmsg, mFu);
		mFu.waitOnFuture();
		LOG_DEBUG("TS :Transaction Complete\n");
	}
}
void TestSpeed::writeOperation(std::string ids[], int size) {
	int nodeCount = NetworkManager::getNodeCount();
	for (int i=0 ; i < 7 ; i++) {
		HyflowMessageFuture mFu;
		std::string id("0-0");
		HyflowMessage hmsg(id);
		hmsg.msg_t = MSG_TYPE_DUMMY;
		DummyTestMsg dmsg("How are you");
		hmsg.setMsg(&dmsg);
		int toNode = i%nodeCount;
		NetworkManager::sendCallbackMessage(toNode, hmsg, mFu);
		mFu.waitOnFuture();
		LOG_DEBUG("TS :Transaction Complete\n");
	}
}

void TestSpeed::checkSanity() {};

std::string* TestSpeed::createLocalObjects(int objectCount) {
	return NULL;
};

} /* namespace vt_dstm */
