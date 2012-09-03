/*
 * TransactionException.cpp
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TransactionException.h"

namespace vt_dstm {

TransactionException::TransactionException() {}
TransactionException::TransactionException(std::string m) { msg = m; }

TransactionException::~TransactionException() throw () {}

} /* namespace vt_dstm */
