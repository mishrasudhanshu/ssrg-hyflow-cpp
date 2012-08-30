/*
 * HyflowObjectFuture.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include "HyflowObjectFuture.h"
#include "directory/DirectoryManager.h"

namespace vt_dstm {

HyflowObjectFuture::HyflowObjectFuture(std::string id, bool rw, unsigned long long tid) {
	objectId = id;
	isRead = rw;
	txnId = tid;
}

HyflowObjectFuture::~HyflowObjectFuture() {}

HyflowObject& HyflowObjectFuture::waitOnObject() {
	messageFuture.waitOnFuture();
	return DirectoryManager::getObjectLocally(objectId,isRead);
}

bool HyflowObjectFuture::isObjectAvailable() {
	return messageFuture.isComplete();
}

} /* namespace vt_dstm */
