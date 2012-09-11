/*
 * TransactionException.h
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TRANSACTIONEXCEPTION_H_
#define TRANSACTIONEXCEPTION_H_

#include <string>
#include "../HyflowException.h"

namespace vt_dstm {

class TransactionException: public vt_dstm::HyflowException {
	std::string msg;
public:
	TransactionException();
	TransactionException(std::string msg);
	virtual ~TransactionException() throw ();
	virtual void print();
};

} /* namespace vt_dstm */

#endif /* TRANSACTIONEXCEPTION_H_ */
