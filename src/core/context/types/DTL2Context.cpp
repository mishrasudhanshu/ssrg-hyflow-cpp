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
#include "../../../util/messages/types/ReadValidationMsg.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../LockTable.h"
#include "../ContextManager.h"

namespace vt_dstm {


DTL2Context::DTL2Context() {
	status = TXN_ACTIVE;
	highestSenderClock = 0;
	tnxClock = ContextManager::getClock();
	txnId = 0;
	nestingModel = ContextManager::getNestingModel();
	parentContext = NULL;
	rootContext = NULL;
	contextExecutionDepth = -1;
}

DTL2Context::~DTL2Context() {
	cleanAllMaps();
}

void DTL2Context::cleanAllMaps(){
	//Delete all heap objects created temporarily
	for (std::map<std::string, HyflowObject*>::iterator i= readMap.begin(); i != readMap.end(); i++ ) {
		if (i->second) {
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	readMap.clear();

	for (std::map<std::string, HyflowObject*>::iterator i= writeMap.begin(); i != writeMap.end(); i++ ) {
		if (i->second){
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	writeMap.clear();

	for (std::map<std::string, HyflowObject*>::iterator i= publishMap.begin(); i != publishMap.end(); i++ ) {
		if (i->second) {
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	publishMap.clear();

	for (std::map<std::string, HyflowObject*>::iterator i= deleteMap.begin(); i != deleteMap.end(); i++ ) {
		if (i->second) {
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	deleteMap.clear();
}

void DTL2Context::contextInit(){
	if (contextExecutionDepth > 0) {
		LOG_DEBUG("DTL : Context already initialize\n");
	} else{
		cleanAllMaps();
		lockSet.clear();

		status = TXN_ACTIVE;
		highestSenderClock = 0;
		tnxClock = ContextManager::getClock();

		//Unregister old txnId, generate new transaction Id and register that
		if( txnId!=0 ) {	// Unregister only if we are using old context
			ContextManager::unregisterContext(this);
		}
		txnId = ContextManager::createTid();
		ContextManager::registerContext(this);

		LOG_DEBUG("DTL : context initialize with id %llu\n", txnId);
	}
}

/*
 * Add the given object copy in read set. No need to create a copy for read set as
 * it will not manipulate the original object
 */
void DTL2Context::beforeReadAccess(HyflowObject *obj) {
	// Perform early validation step : Not required though
	forward(highestSenderClock);

	std::string id = obj->getId();

	// check if object is already part of read set, if not add to read set
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
//				abort();
				TransactionException forwardingFailed("Forward : Aborting on version\n");
				throw forwardingFailed;
			}
		}
		LOG_DEBUG("Forward : context from %d to %d\n", tnxClock, senderClock);
		tnxClock = senderClock;
	}
}

const HyflowObject* DTL2Context::onReadAccess(HyflowObject *obj){
	// Verify in write set whether we have recent value
	std::string id = obj->getId();
	std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
	if ( i == writeMap.end())
		return readMap.at(id);
	return writeMap.at(id);
}

const HyflowObject* DTL2Context::onReadAccess(std::string id){
	// Verify in write set whether we have recent value
	std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
	if ( i == writeMap.end())
		return readMap.at(id);
	return writeMap.at(id);
}

/*
 * Save the copy of object in write set and return copy to transaction to manipulate
 */
HyflowObject* DTL2Context::onWriteAccess(HyflowObject *obj){
	if (getStatus() != TXN_ABORTED) {
		std::string id = obj->getId();
		HyflowObject *writeSetCopy = NULL;
		std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
		if ( i == writeMap.end()) {
			obj->getClone(&writeSetCopy);
			writeMap[id] = writeSetCopy;
			return writeSetCopy;
		}
		return writeMap.at(id);
	}
	return NULL;
}

HyflowObject* DTL2Context::onWriteAccess(std::string id){
	if (getStatus() != TXN_ABORTED) {
		HyflowObject *writeSetCopy = NULL;
		std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
		if ( i == writeMap.end()) {
			readMap.at(id)->getClone(&writeSetCopy);
			writeMap[id] = writeSetCopy;
			return writeSetCopy;
		}
		return writeMap.at(id);
	}
	return NULL;
}

bool DTL2Context::lockObject(HyflowObject* obj) {
	int myNode = NetworkManager::getNodeId();
	if (myNode == obj->getOwnerNode()) {
		LOG_DEBUG("DTL : Local Lock available for %s\n", obj->getId().c_str());
		return LockTable::tryLock(obj->getId(), obj->getVersion(), txnId);
	}else {
		const std::string & objId = obj->getId();
		HyflowMessageFuture mFu;
		HyflowMessage hmsg(objId);
		hmsg.init(MSG_LOCK_ACCESS, true);
		LockAccessMsg lamsg(objId, obj->getVersion(), txnId);
		lamsg.setLock(true);
		lamsg.setRequest(true);
		hmsg.setMsg(&lamsg);
		int owner = obj->getOwnerNode();
		LOG_DEBUG("DTL : Requesting lock for %s from %d of version %d\n", obj->getId().c_str(), owner, obj->getVersion());
		NetworkManager::sendCallbackMessage(owner, hmsg, mFu);
		mFu.waitOnFuture();
		return mFu.isBoolResponse();
	}
}

/*
 * Called to unlock object after failed transaction, unlock local and remote all
 */
void  DTL2Context::unlockObjectOnFail(HyflowObject *obj) {
	int myNode = NetworkManager::getNodeId();
	if (myNode == obj->getOwnerNode()) {
		LOG_DEBUG("DTL : Local Unlock available for %s\n", obj->getId().c_str());
		LockTable::tryUnlock(obj->getId(), obj->getVersion(), txnId);
	}else {
		const std::string & objId = obj->getId();
		HyflowMessage hmsg(objId);
		hmsg.init(MSG_LOCK_ACCESS, false);
		LockAccessMsg lamsg(objId, obj->getVersion(), txnId);
		lamsg.setLock(false);
		lamsg.setRequest(true);
		hmsg.setMsg(&lamsg);
		NetworkManager::sendMessage(obj->getOwnerNode(), hmsg);
	}
}

/*
 * Called to unlock object after successful transaction
 * If objects new owner and old owner are different means remote object. Don't
 * unlock object as change of ownership will automatically tackle it. If old
 * owner and new owner are same, then it is local object so unlock object.
 */
void DTL2Context::unlockObject(HyflowObject* obj) {
	if (obj->getOldOwnerNode() == obj->getOwnerNode()) {
		LockTable::tryUnlock(obj->getId(), obj->getOldHyVersion(), txnId);
	}
}

bool DTL2Context::validateObject(HyflowObject* obj)	{
	/*
	 * Here we don't try to get the object owner first and then compare object
	 * version. We send the validation request to owner known to object itself.
	 * Even if object owner is changed, we will either see locked object or
	 * update object version, allowing us to rollback transaction
	 */
	int owner = obj->getOwnerNode();
	if (owner == NetworkManager::getNodeId()) {
		// If I am the owner verify the object state locally
		HyflowObject* currentObject  = DirectoryManager::getObjectLocally(obj->getId(), false);
		if ( (obj->getVersion() == currentObject->getVersion()) && !LockTable::isLocked(obj->getId(), obj->getVersion(), txnId)) {
			return true;
		}
		return false;
	}else {
		HyflowMessageFuture mFu;
		HyflowMessage hmsg(obj->getId());
		hmsg.msg_t = MSG_READ_VALIDATE;
		ReadValidationMsg rvmsg(obj->getId(), obj->getVersion(), true, txnId);
		hmsg.setMsg(&rvmsg);
		LOG_DEBUG("DTL : Requesting validation for %s from %d of version %d\n", obj->getId().c_str(), owner, obj->getVersion());
		NetworkManager::sendCallbackMessage(owner, hmsg, mFu);
		mFu.waitOnFuture();
		return mFu.isBoolResponse();
	}
	return false;
}

void DTL2Context::tryCommit() {
	std::vector<HyflowObject *> lockedObjects;

	if (getStatus() == TXN_ABORTED) {
		LOG_DEBUG ("Commit : transaction is already aborted\n");
		TransactionException alreadyAborted("Commit: Transaction Already aborted by forwarding\n");
		throw alreadyAborted;
	}

	try {
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator wi;
		// Try to acquire the locks on object in lazy fashion
		// FIXME: Make it asynchronous
		for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
			if (!lockObject(wi->second)) {
				LOG_DEBUG("Commit: Unable to get WriteLock for %s\n", wi->first.c_str());
				TransactionException unableToWriteLock("Commit: Unable to get WriteLock for "+wi->first + "\n");
				throw unableToWriteLock;
			}
			lockedObjects.push_back(wi->second);
			// FIXME: Redirect all the locate call of object through context
			// Remove same object from readSet as we use version locks
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator itr = readMap.find(wi->first) ;
			if ( itr != readMap.end()) {
				HyflowObject *readObject = itr->second;
				itr->second = NULL;
				readMap.erase(itr);
				delete readObject;
			}
		}
		LOG_DEBUG("Commit : Lock Acquisition complete, verifying read Set\n");
		// Perform context
		forward(highestSenderClock);
		// Try to verify read versions of all the objects.
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator ri;
		for ( ri = readMap.rbegin() ; ri != readMap.rend() ; ri++) {
			if (!validateObject(ri->second)) {
				LOG_DEBUG("Commit: Unable to validate for %s, version %d with txn %ull\n", ri->first.c_str(), ri->second->getVersion(), txnId);
				TransactionException readValidationFail("Commit: Unable to validate for "+ri->first+"\n");
				throw readValidationFail;
			}
		}
	} catch (TransactionException& e) {
		// Free all acquired locks
		LOG_DEBUG("Commit: Transaction failed, freeing the locks\n");
		std::vector<HyflowObject *>::iterator vi;
		for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++)
			unlockObjectOnFail(*vi);
		throw;
	}
}

void DTL2Context::reallyCommit() {
	// Transaction Completed Successfully
	// Increase the Node clock
	ContextManager::atomicIncreaseClock();

	// Register yourself as owner of write set objects
	std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator wi;
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		// Update object version
		int version = ContextManager::getClock();
		// LESSON : Use node clock instead of transaction clock to make sure to
		// transaction threads set different object version after commit and object
		// version must increase after each commit.
//		wi->second->setVersion(tnxClock);
		wi->second->setVersion(version);
		LOG_DEBUG("Set object %s version %d\n",wi->first.c_str(), tnxClock);
		// Register object
		DirectoryManager::registerObject(wi->second, txnId);
	}

	// Publish new created objects and wait of ownership change
	for( wi = publishMap.rbegin() ; wi != publishMap.rend() ; wi++ ) {
		DirectoryManager::registerObject(wi->second, txnId);
	}

	// Unregister deleted objects
	for( wi = deleteMap.rbegin() ; wi != deleteMap.rend() ; wi++ ) {
		DirectoryManager::unregisterObject(wi->second, txnId);
	}

	// Release all held locks by this transaction
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		unlockObject(wi->second);
	}
}

void DTL2Context::commit(){
	if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		if (getContextExecutionDepth() > 0) {
			// If not top context do nothing, just return reduce execution depth
			decreaseContextExecutionDepth();
			LOG_DEBUG("DTL : FLAT Context Call, actual commit postponed\n");
			return;
		}
		tryCommit();
		reallyCommit();
		decreaseContextExecutionDepth();
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		Logger::fatal("DTL : Close nesting not supported currently\n");
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("DTL : Open nesting not supported currently\n");
	}else {
		Logger::fatal("DTL : Invalid Nesting Model\n");
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
/*
 * TODO: Provide a method to revive fresh copies
 * Object fetching and beforeRead step combined
 */
void DTL2Context::fetchObject(std::string id) {
	// check if object is already part of read set, if not fetch add to read set
	std::map<std::string, HyflowObject*>::iterator i = readMap.find(id);
	if ( i == readMap.end()) {
		HyflowObject* obj = DirectoryManager::locate(id, true, txnId);
		LOG_DEBUG("DTL : Fetched object %s\n", obj->getId().c_str());
		readMap[id] = obj;
	}

	// Perform early validation step : Not required though
	forward(highestSenderClock);
}

void DTL2Context::fetchObjects(std::string ids[], int objCount) {
	for(int i=0; i < objCount; i++) {
		fetchObject(ids[i]);
	}
}

} /* namespace vt_dstm */
