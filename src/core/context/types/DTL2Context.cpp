/*
 * dtl2Context.cpp
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#include <vector>
#include "DTL2Context.h"
#include "../../directory/DirectoryManager.h"
#include "../../../util/messages/HyflowMessageFuture.h"
#include "../../../util/messages/types/LockAccessMsg.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"

namespace vt_dstm {


DTL2Context::DTL2Context() {
	status = TXN_ACTIVE;
	highestSenderClock = 0;
	tnxClock = ContextManager::getClock();
}

DTL2Context::~DTL2Context() {}

void DTL2Context::beforeReadAccess(HyflowObject *obj) {
	// Perform early validation step : Not required though
	forward(highestSenderClock);

	std::string id = obj->getId();
	// check if object is already part of read set
	std::map<std::string, HyflowObject*>::iterator i = readMap.find(id);
	if ( i == readMap.end())
		readMap[id] = obj;
}
/*
 * Performs the early validation of object at before read time itself and on finding
 * stale object aborts the transaction.
 */
void DTL2Context::forward(int senderClock) {
	if (tnxClock < senderClock) {
		std::map<std::string, HyflowObject*>::iterator i;
		for (i = readMap.begin(); i != readMap.end(); i++)  {
			/*
			 * Abort because following condition means that I read the object for a Node B
			 * who don't know about Node C transactions which led it to higher clock. It is
			 * every much possible that node B have not get the register object message from
			 * node C and thinking itself object owner and giving me older object and actually
			 * I should get that object from node C. Therefore when I compare Node C object
			 * with node B's clock I should abort the transaction.
			 *
			 * Even though chances of this happening is rare, mostly write lock on opening
			 * object itself will force the transaction to abort
			 */
			int32_t version = i->second->getVersion() ;
			if ( version > senderClock) {
				LOG_DEBUG("Forward : Aborting version %d < senderClock %d\n", version, senderClock);
				abort();
			}
		}
		LOG_DEBUG("Forward : context from %d to %d\n", tnxClock, senderClock);
		tnxClock = senderClock;
	}
}

HyflowObject* DTL2Context::onReadAccess(HyflowObject *obj){
	// Verify in write set whether we are recent value
	// Actually current following is not required as both set points to same
	// object in local cache, still will be useful in nesting etc.
	std::string id = obj->getId();
	std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
	if ( i == writeMap.end())
		return readMap.at(id);
	return writeMap.at(id);
}

HyflowObject* DTL2Context::onWriteAccess(HyflowObject *obj){
	if (getStatus() != TXN_ABORTED) {
		std::string id = obj->getId();
		std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
		if ( i == writeMap.end()) {
			writeMap[id] = obj;
			return obj;
		}
		return writeMap.at(id);
	}
	return NULL;
}

bool DTL2Context::lockObject(HyflowObject* obj) {
	HyflowMessageFuture mFu;
	HyflowMessage hmsg;
	hmsg.init(MSG_LOCK_ACCESS, true);
	LockAccessMsg lamsg(obj->getId(), obj->getVersion());
	lamsg.setLock(true);
	lamsg.setRequest(true);
	hmsg.setMsg(&lamsg);
	int owner = obj->getOwnerNode();
	LOG_DEBUG("DTL : Requesting lock for %s from %d of version %d\n", obj->getId().c_str(), owner, obj->getVersion());
	NetworkManager::sendCallbackMessage(owner, hmsg, mFu);
	mFu.waitOnFuture();
	return mFu.isBoolResponse();
}

/*
 * Called to unlock object after failed transaction, unlock local and remote all
 */
void  DTL2Context::unlockObjectOnFail(HyflowObject *obj) {
	HyflowMessage hmsg;
	hmsg.init(MSG_LOCK_ACCESS, false);
	LockAccessMsg lamsg(obj->getId(), obj->getVersion());
	lamsg.setLock(false);
	lamsg.setRequest(true);
	hmsg.setMsg(&lamsg);
	// Networking will detect it as local call and short circuit it
	NetworkManager::sendMessage(obj->getOwnerNode(), hmsg);
}

/*
 * Called to unlock object after successful transaction
 * If objects new owner and old owner are different means remote object. Don't
 * unlock object as change of ownership will automatically tackle it. If old
 * owner and new owner are same, then it is local object so unlock object.
 */
void DTL2Context::unlockObject(HyflowObject* obj) {
	if (obj->getOldOwnerNode() == obj->getOwnerNode()) {
		HyflowMessage hmsg;
		hmsg.init(MSG_LOCK_ACCESS, false);
		LockAccessMsg lamsg(obj->getId(), obj->getOldHyVersion());
		lamsg.setLock(false);
		lamsg.setRequest(true);
		hmsg.setMsg(&lamsg);
		// Networking will detect it as local call and short circuit it
		NetworkManager::sendMessage(obj->getOwnerNode(), hmsg);
	}
}

bool DTL2Context::validateObject(HyflowObject* obj)	{
	return !(obj->getVersion() > tnxClock);
}

void DTL2Context::commit(){
	std::vector<HyflowObject *> lockedObjects;

	if (getStatus() == TXN_ABORTED) {
		LOG_DEBUG ("Commit : transaction is already aborted\n");
		throw *(new TransactionException("Commit: Transaction Already aborted by forwarding\n"));
	}

	try {
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator wi;
		// Try to acquire the locks on object in lazy fashion
		// FIXME: Make it asynchronous
		for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
			if (!lockObject(wi->second)) {
				LOG_DEBUG("Commit: Unable to get WriteLock for %s\n", wi->first.c_str());
				throw new TransactionException("Commit: Unable to get WriteLock for "+wi->first + "\n");
			}
			lockedObjects.push_back(wi->second);
		}

		// Perform context
		forward(highestSenderClock);
		// Try to verify read versions of all the objects.
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator ri;
		for ( ri = readMap.rbegin() ; ri != readMap.rend() ; ri++) {
			if (!validateObject(ri->second)) {
				LOG_DEBUG("Commit: Unable to validate for %s, version %d with txn %d\n", ri->first.c_str(), ri->second->getVersion(), tnxClock);
				throw new TransactionException("Commit: Unable to validate for "+ri->first+"\n");
			}
		}
	} catch (TransactionException* e) {
		// Free all acquired locks
		std::vector<HyflowObject *>::iterator vi;
		for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++)
			unlockObjectOnFail(*vi);
		throw *e;
	}

	// Transaction Completed Successfully
	// Increase the Node clock
	ContextManager::atomicIncreaseClock();

	// Register yourself as owner of write set objects
	std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator wi;
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		// Update object version
		wi->second->setVersion(tnxClock);
		LOG_DEBUG("Set object %s version %d\n",wi->first.c_str(), tnxClock);
		// Register object
		DirectoryManager::registerObject(*wi->second, txnId);
	}

	// Publish new created objects and wait of ownership change
	for( wi = publishMap.rbegin() ; wi != publishMap.rend() ; wi++ ) {
		DirectoryManager::registerObject(*wi->second, txnId);
	}

	// Unregister deleted objects
	for( wi = deleteMap.rbegin() ; wi != deleteMap.rend() ; wi++ ) {
		DirectoryManager::unregisterObject(*wi->second, txnId);
	}

	// Release all held locks by this transaction
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		unlockObject(wi->second);
	}
}

void DTL2Context::abort() {
	setStatus(TXN_ABORTED);
}

void DTL2Context::updateClock(int c) {
	if (highestSenderClock < c) {
		highestSenderClock = c;
	}
}

} /* namespace vt_dstm */
