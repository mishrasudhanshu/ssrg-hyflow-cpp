/*
 * dtl2Context.cpp
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#include <vector>
#include "DTL2Context.h"
#include "../../directory/DirectoryManager.h"
#include "../../helper/CheckPointProvider.h"
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
	tnxClock = ContextManager::getClock();
	highestSenderClock = tnxClock;
	txnId = 0;
	nestingModel = ContextManager::getNestingModel();
	parentContext = NULL;
	rootContext = NULL;
	contextExecutionDepth = -1;
	restartCount = 0;
	subTxnIndex = 0;
	isWrite = false;
}

DTL2Context::~DTL2Context() {
	LOG_DEBUG("DTL :Destroying Context\n");
	cleanAllMaps();
}

void DTL2Context::cleanAllMaps(){
	LOG_DEBUG("DTL :Cleaning Up context Maps\n");
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
		LOG_DEBUG("DTL :Context already initialize\n");
	} else{
		cleanAllMaps();
		lockSet.clear();

		status = TXN_ACTIVE;
		highestSenderClock = 0;
		tnxClock = ContextManager::getClock();

		txnId = ContextManager::createTid(this);
		ContextManager::registerContext(this);

		LOG_DEBUG("DTL :Context initialize with id %llu\n", txnId);
	}
}

void DTL2Context::contextDeinit() {
	if (contextExecutionDepth <= 0) {
		ContextManager::unregisterContext(this);
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
	// and update checkPoint access point
	std::map<std::string, HyflowObject*>::iterator i = readMap.find(id);
	if ( i == readMap.end()) {
		obj->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
		readMap[id] = obj;
	}
}

/*
 * Don't object copy here as it is created in transaction should deleted in cleanMaps
 */
void DTL2Context::addToPublish(HyflowObject *newObject) {
	//If checkPointing enabled set the checkPoint Index
	newObject->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
	publishMap[newObject->getId()] = newObject;
}

/*
 * Copy the object clone in delete set as it is also added in write set
 */
void DTL2Context::addToDelete(HyflowObject *deleteObject) {
	//If checkPointing enabled set the checkPoint Index
	deleteObject->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
	HyflowObject* delObject = NULL;
	deleteObject->getClone(&delObject);
	deleteMap[deleteObject->getId()] = delObject;
}

/*
 * Performs the early validation of object at before read time itself and on finding
 * stale object aborts the transaction.
 */
void DTL2Context::forward(int senderClock) {
	if ((nestingModel == HYFLOW_NESTING_FLAT)||
			(nestingModel == HYFLOW_NO_NESTING)) {
		if (tnxClock < senderClock) {
			std::map<std::string, HyflowObject*>::iterator i;
			for (i = readMap.begin(); i != readMap.end(); i++) {
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
//			int32_t version = i->second->getVersion();
//			if ( version > senderClock) {
			if (!validateObject(i->second)) {
					LOG_DEBUG(
							"Forward :Aborting version %d < senderClock %d\n", i->second->getVersion(), senderClock);
					setStatus(TXN_ABORTED);
					TransactionException forwardingFailed(
							"Forward :Aborting on version\n");
					throw forwardingFailed;
				}
			}
			LOG_DEBUG("Forward :Context from %d to %d\n", tnxClock, senderClock);
			tnxClock = senderClock;
		}
	}else if (nestingModel == HYFLOW_NESTING_CLOSED) {
		if (tnxClock < senderClock) {
			// Collect all the contexts for this leaf context
			std::vector<HyflowContext*> contextBranch;
			for (HyflowContext* current = this; current != NULL; current =
					current->getParentContext()) {
				contextBranch.push_back(current);
			}

			// TODO: Think about merging all validation request for same node into one
			// Perform all local validation first from top to bottom, as messaging is costly
			int myNodeId = NetworkManager::getNodeId();
			bool isAborting = false;
			for (int contextIndex = contextBranch.size() - 1; contextIndex >= 0;
					contextIndex--) {
				DTL2Context* context =
						(DTL2Context*) contextBranch[contextIndex];
				if (!isAborting && (context->getStatus() != TXN_ABORTED)) {
					for (std::map<std::string, HyflowObject*>::iterator i =
							context->readMap.begin();
							i != context->readMap.end(); i++) {
						HyflowObject* obj = i->second;
						if (obj->getOwnerNode() == myNodeId) {
							if (!validateObject(obj)) {
								isAborting = true;
								LOG_DEBUG(
										"Forward :Aborting version %d < senderClock %d\n", obj->getVersion(), senderClock);
								context->setStatus(TXN_ABORTED);
								break;
							}
						}
					}
				} else {
					if (!isAborting) {
						isAborting = true;
					} else {
						context->setStatus(TXN_ABORTED);
					}
				}
			}

			//If still not aborting try validating the remote objects
			if (!isAborting) {
				for (int contextIndex = contextBranch.size() - 1;
						contextIndex >= 0; contextIndex--) {
					DTL2Context* context =
							(DTL2Context*) contextBranch[contextIndex];
					if (!isAborting && (context->getStatus() != TXN_ABORTED)) {
						for (std::map<std::string, HyflowObject*>::iterator i =
								context->readMap.begin();
								i != context->readMap.end(); i++) {
							HyflowObject* obj = i->second;
							if (obj->getOwnerNode() != myNodeId) {
								if (!validateObject(obj)) {
									isAborting = true;
									LOG_DEBUG(
											"Forward :Aborting version %d < senderClock %d\n", obj->getVersion(), senderClock);
									context->setStatus(TXN_ABORTED);
									break;
								}
							}
						}
					} else {
						if (!isAborting) {
							isAborting = true;
						} else {
							context->setStatus(TXN_ABORTED);
						}
					}
				}
			}

			if (isAborting) {
				TransactionException forwardingFailed(
						"Forward :Aborting on version\n");
				throw forwardingFailed;
			}

			LOG_DEBUG("Forward :Context from %d to %d\n", tnxClock, senderClock);
			tnxClock = senderClock;
		}
	}else if (nestingModel == HYFLOW_NESTING_OPEN) {
		Logger::fatal("FORWARD :Open nesting not supported currently\n");
	}else if (nestingModel == HYFLOW_CHECKPOINTING) {
		if (tnxClock < senderClock) {
			int availableCheckPoint = CheckPointProvider::getCheckPointIndex()+1;
			// Try to verify read versions of all the objects.
			std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator rev_ri = readMap.rbegin();
			while(rev_ri != readMap.rend()) {
				int objectsCheckPoint = rev_ri->second->getAccessCheckPoint();
				if ( objectsCheckPoint >= availableCheckPoint) {
					// As this object was access in to be aborted part of transaction don't validate
					HyflowObject *saveObject = rev_ri->second;
					LOG_DEBUG("ValidateCP :Object %s Copy found in read set with accessIndex %d\n", rev_ri->first.c_str(), objectsCheckPoint);
					rev_ri++;
					readMap.erase(saveObject->getId());
					delete saveObject;
					continue;
				}

				//TODO: Think if we need to remove the same object from write and pull down available
				// check point as tryCommit() : Don't make much sense doing read after write to object
				if (!validateObject(rev_ri->second)) {
					LOG_DEBUG("ValidateCP :Unable to validate for %s, version %d accessIndex %d with txn %ull\n", rev_ri->first.c_str(), objectsCheckPoint, rev_ri->second->getVersion(), txnId);

					if (objectsCheckPoint > 0) {
						availableCheckPoint = objectsCheckPoint;
						LOG_DEBUG("ValidateCP :Got a valid checkPoint %d\n", objectsCheckPoint);
					} else { // No check point available throw exception
						setStatus(TXN_ABORTED);
						TransactionException readValidationFail("Commit :Unable to validate for "+rev_ri->first+"\n");
						throw readValidationFail;
					}
				}
				rev_ri++;
			}

			//Clean Up the write set too
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator wi = writeMap.begin();
			while(wi != writeMap.end()) {
				int objectAccessIndex = wi->second->getAccessCheckPoint();
				if ( objectAccessIndex >= availableCheckPoint) {
					// clean-up and delete
					LOG_DEBUG(" CommitCPva :Got write object %s of accessIndex %d inconsistent\n", wi->first.c_str(), objectAccessIndex);
					HyflowObject *saveObject = wi->second;
					writeMap.erase(wi++);
					delete saveObject;
					continue;
				}else{
					wi++;
				}
			}

			// If we reach till here it means we have a valid checkpoint to restore transaction
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator  ri = readMap.begin();
			while(ri != readMap.end()) {
				int objectAccessIndex = ri->second->getAccessCheckPoint();
				if (objectAccessIndex >= availableCheckPoint) {
					// clean-up and delete
					LOG_DEBUG(" CommitCPva :Got read object %s of accessIndex %d inconsistent\n", wi->first.c_str(), objectAccessIndex);
					HyflowObject *saveObject = ri->second;
					readMap.erase(ri++);
					delete saveObject;
					continue;
				}else{
					ri++;
				}
			}

			if (availableCheckPoint < 1) {
				Logger::fatal("Error in CheckPointing :Transaction must have been aborted\n");
			}else {
				if (availableCheckPoint == CheckPointProvider::getCheckPointIndex()+1) {
					LOG_DEBUG("Forward :Context from %d to %d\n", tnxClock, senderClock);
					tnxClock = senderClock;
				}else {
					LOG_DEBUG("Restarting from checkpoint %d in commit phase\n", availableCheckPoint);
					CheckPointProvider::startCheckPoint(availableCheckPoint);
				}
			}
		}
	}else {
		Logger::fatal("FORWARD :Invalid Nesting Model\n");
	}
}

const HyflowObject* DTL2Context::onReadAccess(HyflowObject *obj){
	// Verify in write set whether we have recent value
	std::string id = obj->getId();
	std::map<std::string, HyflowObject*>::iterator i = writeMap.find(id);
	if ( i == writeMap.end()) {
		LOG_DEBUG("DTL :Getting object %s from readSet\n", id.c_str());
		return readMap.at(id);
	}
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
		LOG_DEBUG("DTL :Local Lock available for %s\n", obj->getId().c_str());
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
		LOG_DEBUG("DTL :Requesting lock for %s from %d of version %d\n", obj->getId().c_str(), owner, obj->getVersion());
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
		LOG_DEBUG("DTL :Local Unlock available for %s\n", obj->getId().c_str());
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
		LOG_DEBUG("DTL :Requesting validation for %s from %d of version %d\n", obj->getId().c_str(), owner, obj->getVersion());
		NetworkManager::sendCallbackMessage(owner, hmsg, mFu);
		mFu.waitOnFuture();
		return mFu.isBoolResponse();
	}
	return false;
}

void DTL2Context::tryCommit() {
	std::vector<HyflowObject *> lockedObjects;

	if (getStatus() == TXN_ABORTED) {
		LOG_DEBUG ("Commit :Transaction is already aborted\n");
		TransactionException alreadyAborted("Commit :Transaction Already aborted by forwarding\n");
		throw alreadyAborted;
	}

	try {
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator wi;
		// Try to acquire the locks on object in lazy fashion
		// TODO: Make it asynchronous
		for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
			isWrite = true;
			if (!lockObject(wi->second)) {
				setStatus(TXN_ABORTED);
				LOG_DEBUG("Commit :Unable to get WriteLock for %s\n", wi->first.c_str());
				TransactionException unableToWriteLock("Commit :Unable to get WriteLock for "+wi->first + "\n");
				throw unableToWriteLock;
			}
			lockedObjects.push_back(wi->second);
			// Redirect all the locate call of object through context
			// Remove same object from readSet as we use version locks
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator itr = readMap.find(wi->first) ;
			if ( itr != readMap.end()) {
				HyflowObject *readObject = itr->second;
				itr->second = NULL;
				readMap.erase(itr);
				delete readObject;
			}
		}
		LOG_DEBUG("Commit :Lock Acquisition complete, verifying read Set\n");
		// FIXME: Don't Perform context forwarding here
//		forward(highestSenderClock);
		// Try to verify read versions of all the objects.
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator ri;
		for ( ri = readMap.rbegin() ; ri != readMap.rend() ; ri++) {
			if (!validateObject(ri->second)) {
				setStatus(TXN_ABORTED);
				LOG_DEBUG("Commit :Unable to validate for %s, version %d with txn %ull\n", ri->first.c_str(), ri->second->getVersion(), txnId);
				TransactionException readValidationFail("Commit :Unable to validate for "+ri->first+"\n");
				throw readValidationFail;
			}
		}
	} catch (TransactionException& e) {
		// Free all acquired locks
		LOG_DEBUG("Commit :Transaction failed, freeing the locks\n");
		std::vector<HyflowObject *>::iterator vi;
		for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++)
			unlockObjectOnFail(*vi);
		throw;
	}
}

//FIXME: Fix it for publish and delete set too!!!!!!
void DTL2Context::tryCommitCP() {
	std::vector<HyflowObject *> lockedObjects;

	if (getStatus() == TXN_ABORTED) {
		LOG_DEBUG ("CommitCP :Transaction is already aborted\n");
		TransactionException alreadyAborted("Commit :Transaction Already aborted by forwarding\n");
		throw alreadyAborted;
	}

	try {
		// Try to acquire the locks on object in lazy fashion
		// TODO: Make it asynchronous
		int availableCheckPoint = CheckPointProvider::getCheckPointIndex()+1;
		// LESSON: Make sure iterator don't get invalidated from erase
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator rev_wi = writeMap.rbegin();
		while(rev_wi != writeMap.rend()) {
			int objectsCheckPoint = rev_wi->second->getAccessCheckPoint();

			// Remove same object from readSet as we use version locks
			int readCopyAccessCheckPoint = 0;
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator itr = readMap.find(rev_wi->first) ;
			if ( itr != readMap.end()) {
				HyflowObject *readObject = itr->second;
				readCopyAccessCheckPoint = itr->second->getAccessCheckPoint();
				LOG_DEBUG("CommitCP :Object %s Copy found in read set with accessIndex %d\n", itr->first.c_str(), readCopyAccessCheckPoint);
				itr->second = NULL;
				readMap.erase(itr);
				delete readObject;
			}

			// Check if taking lock on object can be useful
			if ( objectsCheckPoint >= availableCheckPoint) {
				// Pull down the available checkpoint to readCopy level as version have changed or locked
				if (availableCheckPoint > readCopyAccessCheckPoint) {
					availableCheckPoint = readCopyAccessCheckPoint;
					LOG_DEBUG("CommitCP :Pulling down the availableCheckPoint to %d\n", readCopyAccessCheckPoint);
				}

				// As this object was access in to be aborted part of transaction don't get a lock
				// Remove from write set and clean up memory
				HyflowObject *saveObject = rev_wi->second;
				rev_wi++;
				writeMap.erase(saveObject->getId());
				delete saveObject;
				continue;
			}

			// If we are good to take write lock, set the objectCheckPoint to its readCopy so that
			// on failure to get lock we can directly got back to object read time checkPoint
			if(readCopyAccessCheckPoint < objectsCheckPoint) {
				objectsCheckPoint = readCopyAccessCheckPoint;
			}

			if (!lockObject(rev_wi->second)) {
				LOG_DEBUG("CommitCP :Unable to get WriteLock for %s\n", rev_wi->first.c_str());

				// Check if we have any checkPoint available after this lock failure
				if (objectsCheckPoint > 0) {
					LOG_DEBUG("CommitCP :Got a valid checkPoint %d to come down from %d\n", objectsCheckPoint, availableCheckPoint);
					availableCheckPoint = objectsCheckPoint;
				} else { // No check point available throw exception
					setStatus(TXN_ABORTED);
					TransactionException unableToWriteLock("CommitCP :Unable to get WriteLock for "+rev_wi->first + "\n");
					throw unableToWriteLock;
				}
			}else {
				lockedObjects.push_back(rev_wi->second);
			}
			rev_wi++;
		}

		// Now check if in write set have any object with higher accessCheckPointIndex
		// if so remove it from write set and unlock : It is possible because available
		// checkPointIndex dropping after getting lock on given object
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator wi = writeMap.begin();
		while(wi != writeMap.end()) {
			if (wi->second->getAccessCheckPoint() >= availableCheckPoint) {
				LOG_DEBUG("CommitCP :Found writeSet object %s of aborted part %d, while availableCheckPoint %d\n", wi->first.c_str(), wi->second->getAccessCheckPoint(), availableCheckPoint);
				// Unlock and remove from locks marker
				std::vector<HyflowObject *>::iterator vi;
				for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++) {
					if (wi->first.compare((*vi)->getId())==0) {
						LOG_DEBUG("CommitCP :WriteSet object %s unlock as it was partially aborted\n",wi->first.c_str());
						unlockObjectOnFail(*vi);
						lockedObjects.erase(vi);
						break;
					}
				}
				// clean-up and delete
				HyflowObject *saveObject = wi->second;
				writeMap.erase(wi++);
				delete saveObject;
				continue;
			}else{
				wi++;
			}
		}

		LOG_DEBUG("CommitCP :Lock Acquisition complete, verifying read Set\n");
		// FIXME :Don't Perform context forwarding
//		forward(highestSenderClock);
		// Try to verify read versions of all the objects.
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator rev_ri = readMap.rbegin();
		while (rev_ri != readMap.rend()) {
			int objectsCheckPoint = rev_ri->second->getAccessCheckPoint();
			if ( objectsCheckPoint >= availableCheckPoint) {
				// As this object was access in to be aborted part of transaction don't validate
				LOG_DEBUG("CommitCP :ReadMap object %s is of aborted checkPoint %d\n", rev_ri->first.c_str(), objectsCheckPoint);
				HyflowObject *saveObject = rev_ri->second;
				rev_ri++;
				readMap.erase(saveObject->getId());
				delete saveObject;
				continue;
			}

			if (!validateObject(rev_ri->second)) {
				LOG_DEBUG("CommitCP :Unable to validate for %s, version %d with txn %ull\n", rev_ri->first.c_str(), rev_ri->second->getVersion(), txnId);

				if (objectsCheckPoint > 0) {
					availableCheckPoint = objectsCheckPoint;
					LOG_DEBUG("CommitCP :Got a valid checkPoint %d\n", objectsCheckPoint);
				} else { // No check point available throw exception
					setStatus(TXN_ABORTED);
					TransactionException readValidationFail("Commit :Unable to validate for "+rev_ri->first+"\n");
					throw readValidationFail;
				}
			}
			rev_ri++;
		}

		// If we reach till here it means we have a valid checkpoint to restore transaction
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ri;
		ri = readMap.begin();
		while(ri != readMap.end()) {
			if (ri->second->getAccessCheckPoint() >= availableCheckPoint) {
				// clean-up and delete
				LOG_DEBUG("CommitCP :ReadMap had an invalid object %s\n", ri->first.c_str());
				HyflowObject *saveObject = ri->second;
				readMap.erase(ri++);
				delete saveObject;
				continue;
			}else {
				ri++;
			}
		}

		// Time to remove invalid objects in Publish Set and Delete Set too
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pi;
		pi = publishMap.begin();
		while(pi != publishMap.end()) {
			if (pi->second->getAccessCheckPoint() >= availableCheckPoint) {
				// clean-up and delete
				LOG_DEBUG("CommitCP :PublishMap had an invalid object %s\n", pi->first.c_str());
				HyflowObject *saveObject = pi->second;
				publishMap.erase(pi++);
				delete saveObject;
				continue;
			}else {
				pi++;
			}
		}

		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator di;
		di = deleteMap.begin();
		while(di != deleteMap.end()) {
			if (di->second->getAccessCheckPoint() >= availableCheckPoint) {
				// clean-up and delete
				LOG_DEBUG("CommitCP :DeleteMap had an invalid object %s\n", di->first.c_str());
				HyflowObject *saveObject = di->second;
				deleteMap.erase(di++);
				delete saveObject;
				continue;
			}else {
				di++;
			}
		}

		if (availableCheckPoint < 1) {
			Logger::fatal("Error in CheckPointing: Transaction must have been aborted\n");
		}else {
			if (availableCheckPoint == CheckPointProvider::getCheckPointIndex()+1) {
				LOG_DEBUG("CommitCP :No partial Abort required \n");
			}else {
				//LESSON: Due to possible unordered lock grabbing a liveLock can be created, check if
				// transaction is aborted 3+ times, if so don't restart but abort
				// FIXME: Provide abort threshold as a configuration able option
				if (restartCount > 3) {
					LOG_DEBUG("CommitCP :Too many restarts aborting %ull\n", txnId);
					setStatus(TXN_ABORTED);
					TransactionException tooManyRestarts("CommitCP : Too many restarts\n");
					throw tooManyRestarts;
				}else {
					LOG_DEBUG("CommitCP :Restarting from checkpoint %d in commit phase\n", availableCheckPoint);
					restartCount++;
					CheckPointProvider::startCheckPoint(availableCheckPoint);
				}
			}
		}
	} catch (TransactionException& e) {
		// Free all acquired locks
		LOG_DEBUG("Commit :Transaction failed, freeing the locks\n");
		std::vector<HyflowObject *>::iterator vi;
		for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++)
			unlockObjectOnFail(*vi);
		throw;
	}
}

void DTL2Context::reallyCommit() {
	// Transaction Completed Successfully
	// Increase the Node clock
	if (isWrite) {
		ContextManager::atomicIncreaseClock();
	}

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

void DTL2Context::mergeIntoParents() {
	//Copy read set
	DTL2Context* pContext = (DTL2Context*) parentContext;
	for (std::map<std::string, HyflowObject*>::iterator i= readMap.begin(); i != readMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->readMap.find(i->first);
		if (pi == pContext->readMap.end()) {
			pContext->readMap[i->first] = objCopy;
		}else {
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->readMap[i->first] = objCopy;
			delete pCopy;
		}
	}
	//Copy write set
	for (std::map<std::string, HyflowObject*>::iterator i= writeMap.begin(); i != writeMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->writeMap.find(i->first);
		if (pi == pContext->writeMap.end()) {
			pContext->writeMap[i->first] = objCopy;
		}else {
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->writeMap[i->first] = objCopy;
			delete pCopy;
		}
	}
	//Copy publish set
	for (std::map<std::string, HyflowObject*>::iterator i= publishMap.begin(); i != publishMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->publishMap.find(i->first);
		if (pi == pContext->publishMap.end()) {
			pContext->publishMap[i->first] = objCopy;
		}else {
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->publishMap[i->first] = objCopy;
			delete pCopy;
		}
	}
	//Copy delete set
	for (std::map<std::string, HyflowObject*>::iterator i= deleteMap.begin(); i != deleteMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->deleteMap.find(i->first);
		if (pi == pContext->deleteMap.end()) {
			pContext->deleteMap[i->first] = objCopy;
		}else {
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->deleteMap[i->first] = objCopy;
			delete pCopy;
		}
	}
}

void DTL2Context::commit(){
	if (ContextManager::getNestingModel() == HYFLOW_NO_NESTING) {
		// Top context commit here
		tryCommit();
		reallyCommit();
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		if (getContextExecutionDepth() > 0) {
			// If not top context do nothing
			LOG_DEBUG("DTL :FLAT Context Call, actual commit postponed\n");
		} else {	// Top context commit here
			tryCommit();
			reallyCommit();
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		if (!parentContext) {	// Top context commit here
			tryCommit();
			reallyCommit();
		} else {
			LOG_DEBUG("DTL :CLOSED Context Call, merging sets to parent\n");
			mergeIntoParents();
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("DTL :Open nesting not supported currently\n");
	}else if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		tryCommitCP();
		reallyCommit();
	}else{
		Logger::fatal("DTL :Invalid Nesting Model\n");
	}
}

void DTL2Context::rollback() {
	// Currently nothing to do
}

/*
 * It should be called only if transaction is in aborted status
 * It return true, if context requires to throw an transaction Exception
 */
bool DTL2Context::checkParent() {
	if (ContextManager::getNestingModel() == HYFLOW_NESTING_FLAT) {
		if (getContextExecutionDepth() > 0) {
			LOG_DEBUG("DTL :Check Parent throwing exception\n");
			return true;
		} else {	// Top context retry here
			LOG_DEBUG("DTL :Check Parent not throwing exception\n");
			return false;
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_CLOSED) {
		if (!parentContext) {	// Top context commit here
			LOG_DEBUG("DTL :Check Parent not throwing exception\n");
			return false;
		} else {
			// If parent transaction is also aborted then we can not
			// retry on same level therefore throw exception
			if (parentContext->getStatus() == TXN_ABORTED) {
				LOG_DEBUG("DTL :Check Parent throwing exception\n");
				return true;
			} else {
				// if my parent is not aborted then I should keep retrying on my level
				LOG_DEBUG("DTL :Check Parent not throwing exception as parent Active\n");
				return false;
			}
		}
	}else if (ContextManager::getNestingModel() == HYFLOW_NESTING_OPEN) {
		Logger::fatal("DTL :Open nesting not supported currently\n");
	}else {
		Logger::fatal("DTL :Invalid Nesting Model\n");
	}
	return false;
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
void DTL2Context::fetchObject(std::string id, bool isRead=true) {
	// check if object is already part of read set, if not fetch add to read set
	std::map<std::string, HyflowObject*>::iterator i = readMap.find(id);
	if ( i != readMap.end()) {
		// We already have object in transactions read set nothing to do return
		LOG_DEBUG("DTL :Object %s already available in ReadSet\n", id.c_str());
		return;
	}

	//Check if object copy is available in any of parent transaction
	for (DTL2Context* current= (DTL2Context*)parentContext; current != NULL; current = (DTL2Context*)current->parentContext) {
		std::map<std::string, HyflowObject*>::iterator pi = current->readMap.find(id);
		if (pi != current->readMap.end()) {
			// We got object in parent transaction
			HyflowObject* pCopy = NULL;
			pi->second->getClone(&pCopy);
			// Update checkPoint access index and Copy to this transaction readSet
			pCopy->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
			if (isRead) {
				readMap[id] = pCopy;
			}else {
				writeMap[id] = pCopy;
 			}
			return ;
		}
	}

	// Go over network and fetch the object
	HyflowObject* obj = DirectoryManager::locate(id, true, txnId);
	// If object got deleted then throw abort exception
	if (obj == NULL) {
		LOG_DEBUG("DTL :Object %s got Deleted, need to delete this transaction\n", id.c_str());
		status = TXN_ABORTED;
		TransactionException objectDeleted("DTL :Object got Deleted, need to delete this transaction\n");
		throw objectDeleted;
	}
	// Perform early validation step, always use highestSendClock, it is update by objectAccessMessage
	// First do forwarding on current readSet then add new object to it
	forward(highestSenderClock);
	LOG_DEBUG("DTL :Fetched object %s\n", obj->getId().c_str());
	obj->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
	if (isRead) {
		LOG_DEBUG("DTL :Adding fetched object %s to readSet\n", id.c_str());
		readMap[id] = obj;
	}else {
		LOG_DEBUG("DTL :Adding fetched object %s to writeSet\n", id.c_str());
		writeMap[id] = obj;
	}
}

void DTL2Context::fetchObjects(std::string ids[], int objCount, bool isRead=true) {
	for(int i=0; i < objCount; i++) {
		fetchObject(ids[i], isRead);
	}
}

} /* namespace vt_dstm */
