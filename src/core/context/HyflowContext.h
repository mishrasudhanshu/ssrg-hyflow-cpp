/*
 * AbstractContext.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTCONTEXT_H_
#define ABSTRACTCONTEXT_H_

#include <map>
#include "../HyflowObject.h"

namespace vt_dstm {

enum TxnStatus {
	TXN_ACTIVE, TXN_BUSY, TXN_ABORTED,
};

class HyflowContext {
protected:
	TxnStatus status;
	unsigned long long txnId;
public:
	HyflowContext() {
		status = TXN_ACTIVE;
	}

	virtual ~HyflowContext() {}

	TxnStatus getStatus() const {
		return status;
	}

	void setStatus(TxnStatus status) {
		this->status = status;
	}

	unsigned long long getTxnId() const {
		return txnId;
	}

	void setTxnId(unsigned long long txnId) {
		this->txnId = txnId;
	}

	//TODO: Do a field level locking in place of object level, Use some STM library
	/**
	 * Throw (TransactionException) if failed on before read-write call for context
	 */
	virtual void beforeReadAccess(HyflowObject *obj) = 0;
	/**
	 * Throw (TransactionException) if failed on read call for context
	 */
	virtual HyflowObject* onReadAccess(HyflowObject *obj) = 0;
	/**
	 * Throw (TransactionException) if failed on write call for context
	 */
	virtual HyflowObject* onWriteAccess(HyflowObject *obj) = 0;
	/*
	 * Throw (TransactionException) if commit Request is failed.
	 */
	virtual void commit() = 0;
	virtual void abort() = 0;
	/*
	 * Required for DTL
	 */
	virtual void updateClock(int c) = 0;
};

} /* namespace vt_dstm */

#endif /* ABSTRACTCONTEXT_H_ */
