/*
 * TransactionException.cpp
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TransactionException.h"
#include "../../../util/logging/Logger.h"

namespace vt_dstm {

TransactionException::TransactionException() {}
TransactionException::TransactionException(std::string m) { msg = m; }

TransactionException::~TransactionException() throw () {}

void TransactionException::print() {
	LOG_DEBUG(msg.c_str());
}

} /* namespace vt_dstm */
