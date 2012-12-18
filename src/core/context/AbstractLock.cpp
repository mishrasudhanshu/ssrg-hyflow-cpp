/*
 * AbstractLock.cpp
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#include "AbstractLock.h"

namespace vt_dstm {

AbstractLock::AbstractLock() {}

AbstractLock::~AbstractLock() {}

bool AbstractLock::isLocked() {
	return false;
}

bool AbstractLock::lock(bool isRead) {
	return false;
}

void AbstractLock::unlock() {

}

void AbstractLock::getTracker() {

}

} /* namespace vt_dstm */
