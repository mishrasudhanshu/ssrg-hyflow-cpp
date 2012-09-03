/*
 * AbstractContext.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTCONTEXT_H_
#define ABSTRACTCONTEXT_H_

namespace vt_dstm {

class HyflowContext {
public:
	HyflowContext() {};
	virtual ~HyflowContext() {};
	/*
	 * Following function throw (TransactionException) if commitRequest
	 * is failed.
	 */
	virtual void commit() = 0;
	/*
	 * Get transaction Id
	 */
	virtual unsigned long long getTransactionId() = 0;
};

} /* namespace vt_dstm */

#endif /* ABSTRACTCONTEXT_H_ */
