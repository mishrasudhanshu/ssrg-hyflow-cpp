/*
 * AbstractLockTable.cpp
 *
 *  Created on: Dec 14, 2012
 *      Author: mishras[at]vt.edu
 */

#include "AbstractLockTable.h"

namespace vt_dstm {

AbstractLockTable::AbstractLockTable() {
	// TODO Auto-generated constructor stub

}

AbstractLockTable::~AbstractLockTable() {
	// TODO Auto-generated destructor stub
}

bool AbstractLockTable::isLocked(std::string lockName) {
	return false;
}

bool AbstractLockTable::tryLock(std::string lockName, AbstractLock & lock, bool isRead) {
	return false;
}

void AbstractLockTable::unlock(std::string lockName) {

}

} /* namespace vt_dstm */
