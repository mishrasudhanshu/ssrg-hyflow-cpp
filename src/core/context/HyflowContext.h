/*
 * AbstractContext.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTCONTEXT_H_
#define ABSTRACTCONTEXT_H_

#include <map>
#include <string>
#include "../HyflowObject.h"
//#include "../helper/Atomic.h"

namespace vt_dstm {

enum TxnStatus {
	TXN_ACTIVE, TXN_BUSY, TXN_ABORTED,
};

enum Hyflow_NestingModel {
	HYFLOW_NO_NESTING,
	HYFLOW_NESTING_FLAT,
	HYFLOW_NESTING_CLOSED,
	HYFLOW_NESTING_OPEN,
	HYFLOW_CHECKPOINTING
};

class HyflowContext {
protected:
	TxnStatus status;
	unsigned long long txnId;
	Hyflow_NestingModel nestingModel;
	HyflowContext* parentContext;
	HyflowContext* rootContext;
	int contextExecutionDepth;
	int subTxnIndex;
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

	virtual void contextInit() = 0;
	virtual void contextDeinit() = 0;
	//TODO: Do a field level locking in place of object level, Use some STM library
	/**
	 * Throw (TransactionException) if failed on before read-write call for context
	 */
	virtual void beforeReadAccess(HyflowObject *obj) = 0;
	/**
	 * Throw (TransactionException) if failed on read call for context
	 * Returns a read-only copy of object to user
	 */
	const virtual HyflowObject* onReadAccess(HyflowObject* obj) = 0;
	const virtual HyflowObject* onReadAccess(std::string id) = 0;
	/**
	 * Throw (TransactionException) if failed on write call for context
	 */
	virtual HyflowObject* onWriteAccess(HyflowObject* obj) = 0;
	virtual HyflowObject* onWriteAccess(std::string id) = 0;
	/*
	 * To publish or delete object
	 */
	virtual void addToPublish(HyflowObject *newObject) = 0;
	virtual void addToDelete(HyflowObject *deleteObject) = 0;
	/*
	 * Throw (TransactionException) if commit Request is failed.
	 */
	virtual void commit() = 0;
	/*
	 * forces the transaction to rollback
	 */
	virtual void rollback() = 0;
	/*
	 * Suggests whether current transaction should continue or throw transaction
	 * exception of higher level of transaction
	 */
	virtual bool checkParent() = 0;
	/*
	 * Required for DTL
	 */
	virtual void updateClock(int c) = 0;
	/*
	 * Forwarding the context in DTL
	 */
	virtual void forward(int senderClock) = 0;
	/*
	 * Fetch Object for this context and adds to readSet
	 */
	virtual bool fetchObject(std::string objId, bool isRead=true, bool abortOnNull=true ) = 0;
	/*
	 * Fetch Objects for this context
	 */
	virtual void fetchObjects(std::string objIds[], int objCount, bool isRead =
			true) = 0;

	int getSubTxnIndex() const {
		return subTxnIndex;
	}

	void setSubTxnIndex(int subTxnIndex) {
		this->subTxnIndex = subTxnIndex;
	}

	int getContextExecutionDepth() const {
		return contextExecutionDepth;
	}

	void setContextExecutionDepth(int contextExecutionDepth) {
		this->contextExecutionDepth = contextExecutionDepth;
	}

	void increaseContextExecutionDepth() {
		contextExecutionDepth++;
	}

	void decreaseContextExecutionDepth() {
		contextExecutionDepth--;
	}

	HyflowContext* getParentContext() {
		return parentContext;
	}

	void setParentContext(HyflowContext* parentContext) {
		this->parentContext = parentContext;
	}

	HyflowContext* getRootContext() {
		return rootContext;
	}

	void setRootContext(HyflowContext* rootContext) {
		this->rootContext = rootContext;
	}
	/**
	 * Provides the open nesting model, a call to add transaction specific abstract lock in abstract lock
	 * Map. It just creates a entry in context abstract lock Map, it don't locks or unlocks it.
	 *
	 * benchObject	: High level of abstraction of benchmark all the named object are available under it
	 * lockName : lockName is created by benchmark to define abstract lock like (add-after-0-1) can be used
	 * in list to lock any addition after 0-1.
	 * readLock : To get read lock or write lock on abstract lock
	 * acquire : To acquire lock true or false
	 */
	virtual void onLockAction(std::string benchObject, std::string lockName, bool readLock, bool acquire) {}
	virtual void setCurrentAction(void* currentAction) {}
};

} /* namespace vt_dstm */

#endif /* ABSTRACTCONTEXT_H_ */
