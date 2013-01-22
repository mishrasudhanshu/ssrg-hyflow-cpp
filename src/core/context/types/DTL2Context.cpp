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
#include "../../../util/messages/types/AbstractLockMsg.h"
#include "../../../util/messages/types/ReadValidationMsg.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../LockTable.h"
#include "../ContextManager.h"
#include "../AbstractLockTable.h"
#include "../../contention/ContentionManager.h"
#include "../../../benchMarks/BenchmarkExecutor.h"

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> DTL2Context::innerAbortCount;

DTL2Context::DTL2Context() {
	status = TXN_ACTIVE;
	tnxClock = ContextManager::getClock();
	highestSenderClock = tnxClock;
	txnId = 0;
	nestingModel = ContextManager::getNestingModel();
	parentContext = NULL;
	rootContext = NULL;
	contextExecutionDepth = -1;
	subTxnIndex = 0;
	isWrite = false;
	currentAction = NULL;
	abortCount = 0;
	startTime = 0;
}

DTL2Context::DTL2Context(Hyflow_NestingModel nm) {
	status = TXN_ACTIVE;
	tnxClock = ContextManager::getClock();
	highestSenderClock = tnxClock;
	txnId = 0;
	nestingModel = nm;
	parentContext = NULL;
	rootContext = NULL;
	contextExecutionDepth = -1;
	subTxnIndex = 0;
	isWrite = false;
	currentAction = NULL;
	abortCount = 0;
	startTime = 0;
}

DTL2Context::~DTL2Context() {
	LOG_DEBUG("DTL :Destroying Context\n");
	cleanAllMaps();
	delete (Atomic*)currentAction;
}

void DTL2Context::cleanAllMaps(){
	LOG_DEBUG("DTL :Cleaning Up context Maps\n");
	//Delete all heap objects created temporarily

	LOG_DEBUG("DTL :CleanAll readSet size %u\n", readMap.size());
	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= readMap.begin(); i != readMap.end(); i++ ) {
		if (i->second) {
			LOG_DEBUG("DTL :Deleting Read object %s at %p\n", i->first.c_str(), i->second);
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	readMap.clear();

	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= writeMap.begin(); i != writeMap.end(); i++ ) {
		if (i->second){
			LOG_DEBUG("DTL :Deleting Write object %s at %p\n", i->first.c_str(), i->second);
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	writeMap.clear();

	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= publishMap.begin(); i != publishMap.end(); i++ ) {
		if (i->second) {
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	publishMap.clear();

	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= deleteMap.begin(); i != deleteMap.end(); i++ ) {
		if (i->second) {
			HyflowObject* saveData = i->second;
			i->second = NULL;
			delete saveData;
		}
	}
	deleteMap.clear();

	for (std::map<std::string, AbstractLock*>::iterator arItr = abstractReadLocks.begin() ; arItr != abstractReadLocks.end() ; arItr++ ) {
		if (arItr->second) {
			AbstractLock* saveReadAbstract = arItr->second;
			arItr->second = NULL;
			delete saveReadAbstract;
		}
	}
	abstractReadLocks.clear();

	for (std::map<std::string, AbstractLock*>::iterator awItr = abstractWriteLocks.begin() ; awItr != abstractWriteLocks.end() ; awItr++ ) {
		if (awItr->second) {
			AbstractLock* saveReadAbstract = awItr->second;
			awItr->second = NULL;
			delete saveReadAbstract;
		}
	}
	abstractWriteLocks.clear();

	for (std::vector<Atomic*>::iterator aItr=actionList.begin() ; aItr != actionList.end() ; aItr++ ) {
		if (*aItr == NULL) {
			Atomic* saveAtomic = *aItr;
			*aItr = NULL;
			delete saveAtomic;
		}
	}
	actionList.clear();

	if (currentAction) {
		Atomic* axn = (Atomic*) currentAction;
		currentAction = NULL;
		delete axn;
	}
}

void DTL2Context::contextReset() {
	cleanAllMaps();
	lockSet.clear();

	status = TXN_ACTIVE;
	highestSenderClock = 0;
	tnxClock = ContextManager::getClock();

	txnId = ContextManager::createTid(this);
	ContextManager::registerContext(this);

	startTime = Logger::getCurrentMicroSec();
	LOG_DEBUG("DTL :Context of model %d initialize with id %llu\n", nestingModel, txnId);
}

void DTL2Context::contextInit(){
	if (contextExecutionDepth > 0) {
		if (nestingModel == HYFLOW_NESTING_FLAT) {
			LOG_DEBUG("DTL :Context already initialize\n");
		}else {
			contextReset();
		}
	} else{
		contextReset();
	}
}

void DTL2Context::contextDeinit() {
	HyflowMetaData txnMeta;
	if (status == TXN_ABORTED) {
		txnMeta.abortedSubTxnTime = Logger::getCurrentMicroSec() - startTime;
		if (parentContext) {
			BenchmarkExecutor::increaseMetaData(HYFLOW_METADATA_ABORTED_SUBTXNS);
			BenchmarkExecutor::updateMetaData(txnMeta, HYFLOW_METADATA_ABORTED_SUBTXN_TIME);
		}

		if ((nestingModel == HYFLOW_NESTING_OPEN)) {
			rollback();
		}

		ContentionManager::deInit(this);
	}else{
		txnMeta.committedSubTxnTime = Logger::getCurrentMicroSec() - startTime;
		if (parentContext) {
			BenchmarkExecutor::increaseMetaData(HYFLOW_METADATA_COMMITED_SUBTXNS);
			BenchmarkExecutor::updateMetaData(txnMeta, HYFLOW_METADATA_COMMITED_SUBTXN_TIME);
		}
	}


	if (nestingModel == HYFLOW_NESTING_FLAT) {
		if (contextExecutionDepth <= 0) {
			ContextManager::unregisterContext(this);
		}
	}else {
		ContextManager::unregisterContext(this);
	}
}

void DTL2Context::setCurrentAction(void* cAction) {
	currentAction = cAction;
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
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i = readMap.find(id);
	if ( i == readMap.end()) {
		obj->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
		readMap[id] = obj;
	}
}

/*
 * Don't object copy here as it is created in transaction should deleted in cleanMaps
 */
void DTL2Context::addToPublish(HyflowObject *newObject) {
	// Set object version to zero for time being, at commit it will set to required
	newObject->setVersion(0);
	//If checkPointing enabled set the checkPoint Index
	newObject->setAccessCheckPoint(CheckPointProvider::getCheckPointIndex());
	LOG_DEBUG("DTL :Added object %s to publish set at checkPointIndex %d\n", newObject->getId().c_str(), newObject->getAccessCheckPoint());
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

void DTL2Context::addAction(void* action) {
	Atomic *axn = (Atomic*)action;
	actionList.push_back(axn);
}

/*
 * TODO: Optimise, no forwarding if objects/node*threads*benchmarkOperands > 10
 * Performs the early validation of object at before read time itself and on finding
 * stale object aborts the transaction.
 */
void DTL2Context::forward(int senderClock) {
	if ((nestingModel == HYFLOW_NESTING_FLAT)||
			(nestingModel == HYFLOW_NO_NESTING) || (nestingModel == HYFLOW_INTERNAL_OPEN) || (nestingModel == HYFLOW_NESTING_OPEN)) {
		if (tnxClock < senderClock) {
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i;
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
					for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i =
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
						for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i =
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
	}else if (nestingModel == HYFLOW_CHECKPOINTING) {
		if (tnxClock < senderClock) {
			int availableCheckPoint = CheckPointProvider::getCheckPointIndex()+1;

			LOG_DEBUG("DTL :ForwardCP readSet size %u\n", readMap.size());
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ri = readMap.begin();
			while(ri != readMap.end()) {
				int objectsCheckPoint = ri->second->getAccessCheckPoint();
				if ( objectsCheckPoint >= availableCheckPoint) {
					// As this object was access in to be aborted part of transaction don't validate
					// Don't need to erase here as cleanSet is called later
//					HyflowObject *saveObject = ri->second;
//					ri->second = NULL;
//					LOG_DEBUG("ValidateCP :Object %s Copy found in read set with accessIndex %d\n", ri->first.c_str(), objectsCheckPoint);
//					readMap.erase(ri++);
//					delete saveObject;
					ri++;
					continue;
				}

				if (!validateObject(ri->second)) {
					LOG_DEBUG("FowardCP :Unable to validate for %s, version %d accessIndex %d with txn %llu\n", ri->first.c_str(), objectsCheckPoint, ri->second->getVersion(), txnId);

					if (objectsCheckPoint > 0) {
						availableCheckPoint = objectsCheckPoint;
						LOG_DEBUG("FowardCP :Got a valid checkPoint %d\n", objectsCheckPoint);
					} else { // No check point available throw exception
						setStatus(TXN_ABORTED);
						TransactionException readValidationFail("ForwardCP :Unable to validate for "+ri->first+"\n");
						throw readValidationFail;
					}
				}
				ri++;
			}


			// Time to remove invalid objects in read set, Write set, Publish Set and Delete Set too
			cleanSetTillCheckPoint(availableCheckPoint);

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
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator wi = writeMap.find(id);
	if ( wi == writeMap.end()) {
		LOG_DEBUG("DTL :Getting object %s from readSet\n", id.c_str());
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ri = readMap.find(id);
		if ( ri == readMap.end() ) {
			LOG_DEBUG("DTL :Object %s no available in read Set, should be through locate\n", id.c_str());
			readMap[id] = obj;
			return obj;
		}
		return readMap.at(id);
	}
	return writeMap.at(id);
}

const HyflowObject* DTL2Context::onReadAccess(std::string id){
	// TODO: Check in delete map if object got deleted, currently we are safe as Fetch is called first
	// Verify in write set whether we have recent value
	LOG_DEBUG("DTL :OnReadObject readSet size %u\n", readMap.size());
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i = writeMap.find(id);
	if ( i == writeMap.end()) {
		LOG_DEBUG("DTL :Getting object %s from readSet\n", id.c_str());
		return readMap.at(id);
	}
	return writeMap.at(id);
}

/*
 * Save the copy of object in write set and return copy to transaction to manipulate
 */
HyflowObject* DTL2Context::onWriteAccess(HyflowObject *obj){
	LOG_DEBUG("DTL :OnWriteObject readSet size %u\n", readMap.size());
	if (getStatus() != TXN_ABORTED) {
		std::string id = obj->getId();
		HyflowObject *writeSetCopy = NULL;
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i = writeMap.find(id);
		if ( i == writeMap.end()) {
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ri = readMap.find(id);
			if ( ri == readMap.end() ) {
				LOG_DEBUG("DTL :Object %s no available in read Set, should be through locate\n", id.c_str());
				writeMap[id] = obj;
				return obj;
			}else {
				ri->second->getClone(&writeSetCopy);
				writeMap[id] = writeSetCopy;
			}
		}
		return writeMap.at(id);
	}
	return NULL;
}

HyflowObject* DTL2Context::onWriteAccess(std::string id){
	// TODO: Check in delete map if object got deleted, currently we are safe as Fetch is called first
	LOG_DEBUG("DTL :OnWriteObject readSet size %u\n", readMap.size());
	if (getStatus() != TXN_ABORTED) {
		HyflowObject *writeSetCopy = NULL;
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i = writeMap.find(id);
		if ( i == writeMap.end()) {
			LOG_DEBUG("DTL :Copying object %s from readSet to writeSet\n", id.c_str());
			readMap.at(id)->getClone(&writeSetCopy);
			writeMap[id] = writeSetCopy;
			return writeSetCopy;
		}
		return writeMap.at(id);
	}
	return NULL;
}

unsigned long long DTL2Context::getRootTxnId() {
	HyflowContext* topContext = this;
	while (topContext->getParentContext()) {
		topContext = topContext->getParentContext();
	}
	return topContext->getTxnId();
}

void DTL2Context::onLockAccess(std::string objname, std::string lockname, bool readlock) {
	std::stringstream absLockNameStr;
	absLockNameStr<<lockname<<"-"<<objname;
	std::string absLockName = absLockNameStr.str();
	// Use topTxnId for abstract locking to stop any possible deadlock between innerTxns
	unsigned long long topTxnId = getRootTxnId();
	AbstractLock* abLock = new AbstractLock(objname, absLockName, topTxnId);
	LOG_DEBUG("DTL :Created new abstractLock %s for %llu\n", absLockName.c_str(), topTxnId);
	addAbstractLock(absLockName, abLock, readlock);
}

void DTL2Context::addAbstractLock(std::string lockName, void* abstractLock, bool read) {
	AbstractLock *absLock = (AbstractLock*) abstractLock;
	std::map<std::string, AbstractLock*>::iterator i;
	if (read) {
		i = abstractReadLocks.find(lockName);
		if ( i != abstractReadLocks.end()) {
			LOG_DEBUG("DTL :Given read abstract lock %s Already exist in context\n", lockName.c_str());
		}
		abstractReadLocks[lockName] = absLock;
	}else {
		i = abstractWriteLocks.find(lockName);
		if ( i != abstractWriteLocks.end()) {
			LOG_DEBUG("DTL :Given write abstract lock %s Already exist in context\n", lockName.c_str());
		}
		abstractWriteLocks[lockName] = absLock;
	}
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

bool DTL2Context::doAbstractLock(AbstractLock* absLock, bool read) {
	int myNode = NetworkManager::getNodeId();
	int tracker = absLock->getTracker();
	if ( myNode == tracker) {
		LOG_DEBUG("DTL :Lock :Got local AbstractLock %s\n", absLock->getLockName().c_str());
		return AbstractLockTable::tryLock(absLock->getLockName(), absLock, read);
	}else {
		LOG_DEBUG("DTL :Lock :Got AbstractLock %s tracker as %d\n", absLock->getLockName().c_str(), tracker);
		HyflowMessageFuture mFu;
		HyflowMessage hmsg(absLock->getLockName());
		hmsg.init(MSG_ABSTRACT_LOCK, true);
		AbstractLockMsg almsg(absLock,txnId, true);
		hmsg.setMsg(&almsg);
		NetworkManager::sendCallbackMessage(tracker, hmsg, mFu);
		mFu.waitOnFuture();
		return mFu.isBoolResponse();
	}
	return false;
}

void DTL2Context::doAbstractUnLock(AbstractLock* absLock, bool read) {
	int myNode = NetworkManager::getNodeId();
	int tracker = absLock->getTracker();
	if ( myNode == tracker) {
		LOG_DEBUG("DTL :Unlock :Got local AbstractLock %s\n", absLock->getLockName().c_str());
		AbstractLockTable::unlock(absLock->getLockName());
	}else {
		LOG_DEBUG("DTL :Unlock :Got AbstractLock %s tracker as %d\n", absLock->getLockName().c_str(), tracker);
		HyflowMessage hmsg(absLock->getLockName());
		hmsg.init(MSG_ABSTRACT_LOCK, false);
		AbstractLockMsg almsg(absLock,txnId, false);
		hmsg.setMsg(&almsg);
		NetworkManager::sendMessage(tracker, hmsg);
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
	if (owner == -1) {
		// While forwarding this condition might come, not in commit as we filter it out
		LOG_DEBUG("Validate :Must be from forwarding, no forwarding required for publish set object\n");
		return true;
	}

	if (owner == NetworkManager::getNodeId()) {
		// If I am the owner verify the object state locally
		HyflowObject* currentObject  = DirectoryManager::getObjectLocally(obj->getId(), false);
		if ( (obj->getVersion() == currentObject->getVersion()) && !LockTable::isLocked(obj->getId(), obj->getVersion(), txnId)) {
			LOG_DEBUG("DTL :Validation of %s version %d successful\n", obj->getId().c_str(), obj->getVersion());
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
		for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
			// Remove same object from readSet as we use version locks
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator itr = readMap.find(wi->first) ;
			if ( itr != readMap.end()) {
				HyflowObject *readObject = itr->second;
				itr->second = NULL;
				readMap.erase(itr);
				delete readObject;
			}

			isWrite = true;
			// If same object exist in publish set of my or another innerTxn don't take lock on it
			// It might have been copied to write Map when manipulated by other transaction
			if (wi->second->getOwnerNode() == -1) {
				LOG_DEBUG("Commit :Publish set object %s, lock not required\n", wi->first.c_str());
				// Copy it to publish set object, delete old publish copy
				// Don't do anything, it may create issue for checkPointing, do all in really commit
				continue;
			}

			if (!lockObject(wi->second)) {
				setStatus(TXN_ABORTED);
				LOG_DEBUG("Commit :Unable to get WriteLock for %s\n", wi->first.c_str());
				TransactionException unableToWriteLock("Commit :Unable to get WriteLock for "+wi->first + "\n");
				throw unableToWriteLock;
			}
			lockedObjects.push_back(wi->second);

		}
		LOG_DEBUG("Commit :Lock Acquisition complete, verifying read Set\n");
		// FIXME: Don't Perform context forwarding here
//		forward(highestSenderClock);
		// Try to verify read versions of all the objects.
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator ri;
		for ( ri = readMap.rbegin() ; ri != readMap.rend() ; ri++) {
			// If same object exist in publish set of my or another innerTxn don't validate
			// It might have been copied to write Map when manipulated by other transaction
			if (ri->second->getOwnerNode() == -1) {
				LOG_DEBUG("Commit :Publish set object %s in read Set validation not required\n", ri->first.c_str());
				continue;
			}

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

void DTL2Context::cleanSetTillCheckPoint(int availableCheckPoint) {
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ri;
	ri = readMap.begin();
	LOG_DEBUG("DTL :CleanSet readSet size %u\n", readMap.size());
	std::vector<std::string> invalidReadObjects;
	while(ri != readMap.end()) {
		if (ri->second->getAccessCheckPoint() >= availableCheckPoint) {
			// clean-up and delete
			LOG_DEBUG("CommitCP :ReadMap had an invalidated object %s and %p\n", ri->first.c_str(), ri->second);
			HyflowObject *saveObject = ri->second;
			ri->second = NULL;
//			readMap.erase(ri++);
			invalidReadObjects.push_back(ri->first);
			ri++;
			delete saveObject;
			continue;
		}else {
			LOG_DEBUG("CommitCP :ReadMap had an validated object %s and %p\n", ri->first.c_str(), ri->second);
			ri++;
		}
	}

	for(std::vector<std::string>::iterator irItr=invalidReadObjects.begin() ; irItr!= invalidReadObjects.end() ; irItr++ ) {
		int del = 0;
		del = readMap.erase(*irItr);
		LOG_DEBUG("CleanSet :ReadMap invalid object %s deleted=%d size=%d\n", irItr->c_str(), del, readMap.size());
	}

	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator wi = writeMap.begin();
	while(wi != writeMap.end()) {
		if (wi->second->getAccessCheckPoint() >= availableCheckPoint) {
			LOG_DEBUG("CommitCP :Found writeSet object %s of aborted part %d, while availableCheckPoint %d at %p\n", wi->first.c_str(), wi->second->getAccessCheckPoint(), availableCheckPoint , wi->second);
			// clean-up and delete
			HyflowObject *saveObject = wi->second;
			wi->second = NULL;
			writeMap.erase(wi++);
			delete saveObject;
			continue;
		}else{
			wi++;
		}
	}

	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pi;
	pi = publishMap.begin();
	while(pi != publishMap.end()) {
		if (pi->second->getAccessCheckPoint() >= availableCheckPoint) {
			// clean-up and delete
			LOG_DEBUG("CommitCP :PublishMap had an invalidated object %s\n", pi->first.c_str());
			HyflowObject *saveObject = pi->second;
			pi->second = NULL;
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
			LOG_DEBUG("CommitCP :DeleteMap had an invalidate object %s\n", di->first.c_str());
			HyflowObject *saveObject = di->second;
			di->second = NULL;
			deleteMap.erase(di++);
			delete saveObject;
			continue;
		}else {
			di++;
		}
	}
}


void DTL2Context::tryCommitCP() {
	std::vector<HyflowObject *> lockedObjects;

	if (getStatus() == TXN_ABORTED) {
		LOG_DEBUG ("CommitCP :Transaction is already aborted\n");
		TransactionException alreadyAborted("Commit :Transaction Already aborted by forwarding\n");
		throw alreadyAborted;
	}

	try {
		// Try to acquire the locks on object in lazy fashion
		int availableCheckPoint = CheckPointProvider::getCheckPointIndex()+1;
		assert(availableCheckPoint);
		// LESSON: Make sure iterator don't get invalidated from erase
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator rev_wi = writeMap.rbegin();
		while(rev_wi != writeMap.rend()) {
			int objectsCheckPoint = rev_wi->second->getAccessCheckPoint();
			isWrite = true;

			// Remove same object from readSet as we use version locks
			int readCopyAccessCheckPoint = 0;
			LOG_DEBUG("DTL :TryCommit readSet size %u\n", readMap.size());
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator itr = readMap.find(rev_wi->first) ;
			if ( itr != readMap.end()) {
				HyflowObject *readObject = itr->second;
				readCopyAccessCheckPoint = itr->second->getAccessCheckPoint();
				LOG_DEBUG("CommitCP :Object %s Copy found in read set with accessIndex %d\n", itr->first.c_str(), readCopyAccessCheckPoint);
				itr->second = NULL;
				readMap.erase(itr);
				delete readObject;
			}

			// If we are good to take write lock, set the objectCheckPoint to its readCopy so that
			// on failure to get lock we can directly got back to object read time checkPoint
			if(readCopyAccessCheckPoint < objectsCheckPoint) {
				objectsCheckPoint = readCopyAccessCheckPoint;
			}

			// If same object exist in publish set of my or another innerTxn don't take lock on it
			// It might have been copied to write Map when manipulated by other transaction
			if (rev_wi->second->getOwnerNode() == -1) {
				LOG_DEBUG("Commit :Publish set object %s, lock not required\n",rev_wi->first.c_str());
				rev_wi++;
				continue;
			}

			// We don't need to try more locks to find best point to return as read validation do the same
			if (!lockObject(rev_wi->second)) {
				LOG_DEBUG("CommitCP :Unable to get WriteLock for %s\n", rev_wi->first.c_str());

				// Check if we have any checkPoint available after this lock failure
				if (objectsCheckPoint > 0) {
					LOG_DEBUG("CommitCP :Got a valid checkPoint %d to come down from %d\n", objectsCheckPoint, availableCheckPoint);
					availableCheckPoint = objectsCheckPoint;
					break;
				} else { // No check point available throw exception
					setStatus(TXN_ABORTED);
					TransactionException unableToWriteLock("CommitCP :Unable to get WriteLock for "+rev_wi->first + "\n");
					throw unableToWriteLock;
				}
			}else {
				HyflowObject* lockCopy = NULL;
				rev_wi->second->getClone(&lockCopy);
				lockedObjects.push_back(lockCopy);
			}
			rev_wi++;
		}

		LOG_DEBUG("CommitCP :Lock Acquisition complete, verifying read Set\n");

		// Try to verify read versions of all the objects, even if lock validation failed
		// we might low down the access check point lower
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ri = readMap.begin();
		while (ri != readMap.end()) {
			int objectsCheckPoint = ri->second->getAccessCheckPoint();
			if ( objectsCheckPoint >= availableCheckPoint) {
				ri++;
				continue;
			}

			// If same object exist in publish set of my or another innerTxn don't validate
			// It might have been copied to write Map when manipulated by other transaction
			if (ri->second->getOwnerNode() == -1) {
				LOG_DEBUG("CommitCP :Publish set object %s in read Set validation not required\n", ri->first.c_str());
				ri++;
				continue;
			}

			if (!validateObject(ri->second)) {
				LOG_DEBUG("CommitCP :Unable to validate for %s, version %d with txn %ull\n", ri->first.c_str(), ri->second->getVersion(), txnId);

				if (objectsCheckPoint > 0) {
					availableCheckPoint = objectsCheckPoint;
					LOG_DEBUG("CommitCP :Got a valid checkPoint %d\n", objectsCheckPoint);
				} else { // No check point available throw exception
					setStatus(TXN_ABORTED);
					TransactionException readValidationFail("Commit :Unable to validate for "+ri->first+"\n");
					throw readValidationFail;
				}
			}
			ri++;
		}

		// If we reach till here it means we have a valid checkpoint to restore transaction
		// Time to remove invalid objects in read set, Write set, Publish Set and Delete Set
		cleanSetTillCheckPoint(availableCheckPoint);

		if (availableCheckPoint < 1) {
			Logger::fatal("Error in CheckPointing: Transaction must have been aborted\n");
			TransactionException checkPointFailure("CommitCP :CheckPoint Not available\n");
			throw checkPointFailure;
		}else {
			if (availableCheckPoint == CheckPointProvider::getCheckPointIndex()+1) {
				LOG_DEBUG("CommitCP :No partial Abort required \n");
			}else {
				// Can not happen any more we also clean locks
				LOG_DEBUG("CommitCP :Restarting from checkpoint %d in commit phase\n", availableCheckPoint);
				std::vector<HyflowObject *>::iterator vi;
				for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++) {
					unlockObjectOnFail(*vi);
					delete *vi;
				}
				lockedObjects.clear();

				CheckPointProvider::startCheckPoint(availableCheckPoint);
			}
		}
	} catch (TransactionException& e) {
		// Free all acquired locks
		LOG_DEBUG("Commit :Transaction failed, freeing the locks\n");
		std::vector<HyflowObject *>::iterator vi;
		for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++) {
			unlockObjectOnFail(*vi);
			delete *vi;
		}
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
	// Update object version
	int version = ContextManager::getClock();
	LOG_DEBUG("DTL :Commit write Set Objects\n");
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		// LESSON : Use node clock instead of transaction clock to make sure to
		// transaction threads set different object version after commit and object
		// version must increase after each commit.
		wi->second->setVersion(version);
		LOG_DEBUG("Set object %s version %d\n",wi->first.c_str(), version);
		// Register object
		DirectoryManager::registerObject(wi->second, txnId);

		// Check if updated object was part of publish set too, if so delete from there as that is older
		// Don't need to it with delete set object as they commit at last
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ti = publishMap.find(wi->first);
		if ( ti != publishMap.end()) {
			LOG_DEBUG("DTL :Deleting from publish set object %s copy\n", wi->first.c_str());
			publishMap.erase(ti);
		}
	}

	// Publish new created objects and wait of ownership change
	LOG_DEBUG("DTL :Commit publish Set Objects\n");
	for( wi = publishMap.rbegin() ; wi != publishMap.rend() ; wi++ ) {
		// Check if this object was deleted in same transaction, if so don't register
		// Actually we can skip as, delete commit happens at last

		// otherwise tackle usually
		wi->second->setVersion(version);
		DirectoryManager::registerObject(wi->second, txnId);
	}

	// Unregister deleted objects
	LOG_DEBUG("DTL :Commit delete Set Objects\n");
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
	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= readMap.begin(); i != readMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->readMap.find(i->first);
		if (pi == pContext->readMap.end()) {
			pContext->readMap[i->first] = objCopy;
		}else {
			LOG_DEBUG("DTL :Merge overwrite %s in parent Read Map\n", i->first.c_str());
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->readMap[i->first] = objCopy;
			delete pCopy;
		}
	}
	//Copy write set
	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= writeMap.begin(); i != writeMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->writeMap.find(i->first);
		if (pi == pContext->writeMap.end()) {
			pContext->writeMap[i->first] = objCopy;
		}else {
			LOG_DEBUG("DTL :Merge overwrite %s in parent write Map\n", i->first.c_str());
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->writeMap[i->first] = objCopy;
			delete pCopy;
		}
	}
	//Copy publish set
	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= publishMap.begin(); i != publishMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->publishMap.find(i->first);
		if (pi == pContext->publishMap.end()) {
			pContext->publishMap[i->first] = objCopy;
		}else {
			LOG_DEBUG("DTL :Merge overwrite %s in parent publish Map\n", i->first.c_str());
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->publishMap[i->first] = objCopy;
			delete pCopy;
		}
	}
	//Copy delete set
	for (std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator i= deleteMap.begin(); i != deleteMap.end(); i++ ) {
		HyflowObject *objCopy = NULL;
		i->second->getClone(&objCopy);
		std::map<std::string, HyflowObject*>::iterator pi =pContext->deleteMap.find(i->first);
		if (pi == pContext->deleteMap.end()) {
			pContext->deleteMap[i->first] = objCopy;
		}else {
			LOG_DEBUG("DTL :Merge overwrite %s in parent delete Map\n", i->first.c_str());
			HyflowObject *pCopy = pi->second;
			//Over write the child object copy
			pContext->deleteMap[i->first] = objCopy;
			delete pCopy;
		}
	}
}

void DTL2Context::tryCommitON() {
	std::vector<HyflowObject *> lockedObjects;
	std::vector<std::string> lockedReadAbstractLock;
	std::vector<std::string> lockedWriteAbstractLock;

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
			// Remove same object from readSet as we use version locks
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator itr = readMap.find(wi->first) ;
			if ( itr != readMap.end()) {
				HyflowObject *readObject = itr->second;
				itr->second = NULL;
				readMap.erase(itr);
				delete readObject;
			}

			isWrite = true;
			// If same object exist in publish set of my or another innerTxn don't take lock on it
			// It might have been copied to write Map when manipulated by other transaction
			if (wi->second->getOwnerNode() == -1) {
				LOG_DEBUG("Commit :Publish set object %s, lock not required\n", wi->first.c_str());
				continue;
			}

			if (!lockObject(wi->second)) {
				setStatus(TXN_ABORTED);
				LOG_DEBUG("Commit :Unable to get WriteLock for %s\n", wi->first.c_str());
				TransactionException unableToWriteLock("Commit :Unable to get WriteLock for "+wi->first + "\n");
				throw unableToWriteLock;
			}
			lockedObjects.push_back(wi->second);

		}
		LOG_DEBUG("Commit :Lock Acquisition complete, verifying read Set\n");
		// FIXME: Don't Perform context forwarding here
//		forward(highestSenderClock);
		// Try to verify read versions of all the objects.
		std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator ri;
		for ( ri = readMap.rbegin() ; ri != readMap.rend() ; ri++) {
			// If same object exist in publish set of my or another innerTxn don't validate
			// It might have been copied to write Map when manipulated by other transaction
			if (ri->second->getOwnerNode() == -1) {
				LOG_DEBUG("Commit :Publish set object %s in read Set validation not required\n", ri->first.c_str());
				continue;
			}

			if (!validateObject(ri->second)) {
				setStatus(TXN_ABORTED);
				LOG_DEBUG("Commit :Unable to validate for %s, version %d with txn %ull\n", ri->first.c_str(), ri->second->getVersion(), txnId);
				TransactionException readValidationFail("Commit :Unable to validate for "+ri->first+"\n");
				throw readValidationFail;
			}
		}
		// Obtain the abstract locks
		std::map<std::string, AbstractLock*>::iterator arItr;
		for ( arItr = abstractReadLocks.begin() ; arItr != abstractReadLocks.end() ; arItr++) {
			if (!arItr->second->isLocked()) {
				if (!doAbstractLock(arItr->second, true)) {
					setStatus(TXN_ABORTED);
					LOG_DEBUG("Commit :Unable to get Atomic AbstractReadLock for %s\n", arItr->first.c_str());
					TransactionException unableToAbsReadLock("Commit :Unable to get abstract readLock for "+arItr->first + "\n");
					for (HyflowContext* pc = parentContext ; pc!=NULL ; pc = pc->getParentContext() ) {
						pc->setStatus(TXN_ABORTED);
					}
					throw unableToAbsReadLock;
				}
				lockedReadAbstractLock.push_back(arItr->first);
			}
		}
		std::map<std::string, AbstractLock*>::iterator awItr;
		for ( awItr = abstractWriteLocks.begin() ; awItr != abstractWriteLocks.end() ; awItr++) {
			if (!awItr->second->isLocked()) {
				if (!doAbstractLock(awItr->second, false)) {
					setStatus(TXN_ABORTED);
					LOG_DEBUG("Commit :Unable to get AbstractWriteLock for %s\n", awItr->first.c_str());
					TransactionException unableToAbsWriteLock("Commit :Unable to get abstract writeLock for "+awItr->first + "\n");
					// We will can not retry abort context tree
					for (HyflowContext* pc = parentContext ; pc!=NULL ; pc = pc->getParentContext() ) {
						pc->setStatus(TXN_ABORTED);
					}
					throw unableToAbsWriteLock;
				}
				lockedWriteAbstractLock.push_back(awItr->first);
			}
		}
	} catch (TransactionException& e) {
		// Perform Rollback
		rollback();

		// Free all acquired locks
		LOG_DEBUG("Commit :Transaction failed, freeing the locks\n");
		std::vector<HyflowObject *>::iterator vi;
		for ( vi = lockedObjects.begin(); vi != lockedObjects.end(); vi++) {
			unlockObjectOnFail(*vi);
		}
		for (std::vector<std::string>::iterator rabsItr = lockedReadAbstractLock.begin() ; rabsItr != lockedReadAbstractLock.end() ; rabsItr++ ) {
			doAbstractUnLock(abstractReadLocks.at(*rabsItr), true);
		}
		for (std::vector<std::string>::iterator wabsItr = lockedWriteAbstractLock.begin() ; wabsItr != lockedWriteAbstractLock.end() ; wabsItr++ ) {
			doAbstractUnLock(abstractWriteLocks.at(*wabsItr), false);
		}
		throw;
	}
}

void DTL2Context::reallyCommitON() {
	// Transaction Completed Successfully
	// Increase the Node clock
	if (isWrite) {
		ContextManager::atomicIncreaseClock();
	}

	// Register yourself as owner of write set objects
	std::map<std::string, HyflowObject*, ObjectIdComparator>::reverse_iterator wi;
	// Update object version
	int version = ContextManager::getClock();
	LOG_DEBUG("DTL :Commit write Set Objects\n");
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		// LESSON : Use node clock instead of transaction clock to make sure to
		// transaction threads set different object version after commit and object
		// version must increase after each commit.
		wi->second->setVersion(version);
		LOG_DEBUG("Set object %s version %d\n",wi->first.c_str(), version);
		// Register object
		DirectoryManager::registerObject(wi->second, txnId);

		// Check if updated object was part of publish set too, if so delete from there as that is older
		// Don't need to it with delete set object as they commit at last
		std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ti = publishMap.find(wi->first);
		if ( ti != publishMap.end()) {
			LOG_DEBUG("DTL :Deleting from publish set object %s copy\n", wi->first.c_str());
			publishMap.erase(ti);
		}
	}

	// Publish new created objects and wait of ownership change
	LOG_DEBUG("DTL :Commit publish Set Objects\n");
	for( wi = publishMap.rbegin() ; wi != publishMap.rend() ; wi++ ) {
		// Check if this object was deleted in same transaction, if so don't register
		// Actually we can skip as, delete commit happens at last

		// Check if this object was updated later and is part of write set, don't publish old copy
		// While tryCommit we copy the latest copy of object to publish set
//		ti = writeMap.find(wi->first);
//		if ( ti != writeMap.end()) {
//			LOG_DEBUG("DTL :Not publishing Old object copy %s, it is published in writeSet\n", wi->first.c_str());
//			continue;
//		}

		// otherwise tackle usually
		wi->second->setVersion(version);
		DirectoryManager::registerObject(wi->second, txnId);
	}

	// Unregister deleted objects
	LOG_DEBUG("DTL :Commit delete Set Objects\n");
	for( wi = deleteMap.rbegin() ; wi != deleteMap.rend() ; wi++ ) {
		DirectoryManager::unregisterObject(wi->second, txnId);
	}

	// Commit the Inner transaction abstract lock and actions to parent
	if (contextExecutionDepth == 0) {
		// Release all the abstract locks
		for ( std::map<std::string, AbstractLock*>::iterator arItr = abstractReadLocks.begin() ; arItr != abstractReadLocks.end() ; arItr++) {
			doAbstractUnLock(arItr->second, true);
		}
		for ( std::map<std::string, AbstractLock*>::iterator awItr = abstractWriteLocks.begin() ; awItr != abstractWriteLocks.end() ; awItr++) {
			doAbstractUnLock(awItr->second, false);
		}
	}else {
		// Merge locks and actions into parents
		if (parentContext) {
			// Abstract Read locks
			LOG_DEBUG("DTL :Merge the Abstract Locks and Actions at depth %d\n", contextExecutionDepth);
			for ( std::map<std::string, AbstractLock*>::iterator arItr = abstractReadLocks.begin() ; arItr != abstractReadLocks.end() ; arItr++) {
				AbstractLock *absRCopy = NULL;
				arItr->second->getClone(&absRCopy);
				absRCopy->setAbsLock(true);
				parentContext->addAbstractLock(arItr->first, absRCopy, true);
			}
			// Abstract Write locks
			for ( std::map<std::string, AbstractLock*>::iterator awItr = abstractWriteLocks.begin() ; awItr != abstractWriteLocks.end() ; awItr++) {
				AbstractLock *absWCopy = NULL;
				awItr->second->getClone(&absWCopy);
				absWCopy->setAbsLock(true);
				parentContext->addAbstractLock(awItr->first, absWCopy, false);
			}
			// current Actions
			actionList.push_back((Atomic*)currentAction);

			for ( std::vector<Atomic*>::iterator aItr = actionList.begin() ;  aItr != actionList.end() ; aItr++ ) {
				Atomic* actionCopy = NULL;
				Atomic* actual = *aItr;
				actual->getClone(&actionCopy);
				actionCopy->setCompleted(true);
				parentContext->addAction(actionCopy);
			}
		}else {
			Logger::fatal("DTL :Parent Context of open nested inner txn not found\n");
		}
	}

	// Release all held locks by this transaction
	for( wi = writeMap.rbegin() ; wi != writeMap.rend() ; wi++ ) {
		unlockObject(wi->second);
	}
}

void DTL2Context::commit(){
	if ((nestingModel == HYFLOW_NO_NESTING) || (nestingModel == HYFLOW_INTERNAL_OPEN)) {
		// Top context commit here
		tryCommit();
		reallyCommit();
	}else if (nestingModel == HYFLOW_NESTING_FLAT) {
		if (getContextExecutionDepth() > 0) {
			// If not top context do nothing
			LOG_DEBUG("DTL :FLAT Context Call, actual commit postponed\n");
		} else {	// Top context commit here
			tryCommit();
			reallyCommit();
		}
	}else if (nestingModel == HYFLOW_NESTING_CLOSED) {
		if (!parentContext) {	// Top context commit here
			tryCommit();
			reallyCommit();
		} else {
			LOG_DEBUG("DTL :CLOSED Context Call, merging sets to parent\n");
			mergeIntoParents();
			resetInnerAbortCount();
		}
	}else if (nestingModel == HYFLOW_NESTING_OPEN) {
		tryCommitON();
		// merge inner transaction actions in parent
		reallyCommitON();
		if (parentContext) {
			resetInnerAbortCount();
		}
	}else if (nestingModel == HYFLOW_CHECKPOINTING) {
		tryCommitCP();
		reallyCommit();
	}else{
		Logger::fatal("DTL :Commit call faced Invalid Nesting Model\n");
	}
}

void DTL2Context::rollback() {
	if (nestingModel == HYFLOW_NESTING_OPEN) {
		for ( std::vector<Atomic*>::iterator sItr = actionList.begin() ; sItr != actionList.end() ; sItr++ ) {
			Atomic* abortAction = *sItr;
			if (abortAction->isCompleted()) {
				LOG_DEBUG("DTL :Performing ROLLBACK action\n");
				Atomic atomicAction(HYFLOW_INTERNAL_OPEN);
				atomicAction.atomically = abortAction->onAbort;
				atomicAction.execute(NULL, abortAction->getArguements(), abortAction->getReturnValue());
			}
			abortAction->setCompleted(false);
		}

		for (std::map<std::string, AbstractLock*>::iterator arItr = abstractReadLocks.begin() ; arItr != abstractReadLocks.end() ; arItr++ ) {
			if (arItr->second->isLocked()) {
				doAbstractUnLock(arItr->second, true);
			}
			arItr->second->setAbsLock(false);
		}

		for (std::map<std::string, AbstractLock*>::iterator awItr = abstractWriteLocks.begin() ; awItr != abstractWriteLocks.end() ; awItr++ ) {
			if (awItr->second->isLocked()) {
				doAbstractUnLock(awItr->second, true);
			}
			awItr->second->setAbsLock(false);
		}
	}
}

/*
 * It should be called only if transaction is in aborted status
 * It return true, if context requires to throw an transaction Exception
 */
bool DTL2Context::checkParent() {
	if (nestingModel == HYFLOW_NESTING_FLAT) {
		if (getContextExecutionDepth() > 0) {
			LOG_DEBUG("DTL :Check Parent throwing exception\n");
			return true;
		} else {	// Top context retry here
			LOG_DEBUG("DTL :Check Parent not throwing exception\n");
			return false;
		}
	}else if (nestingModel == HYFLOW_NESTING_CLOSED) {
		if (!parentContext) {	// Top context commit here
			LOG_DEBUG("DTL :Top context Check Parent not throwing exception\n");
			return false;
		} else {
			// If parent transaction is also aborted then we can not
			// retry on same level therefore throw exception
			if (parentContext->getStatus() == TXN_ABORTED) {
				LOG_DEBUG("DTL :Check Parent throwing exception\n");
				return true;
			} else {
				// Inner transaction abort count
				increaseInnerAbortCount();
				int aborts = getInnerAbortCount();
				if (aborts > 3 ) {
					LOG_DEBUG("DTL :Repeated Inner transaction Abort=%d, full abort\n", aborts);
					resetInnerAbortCount();
					return true;
				}
				LOG_DEBUG("DTL :Check Parent not throwing exception as parent Active, abort Count %d\n", aborts);
				return false;
			}
		}
	}else if (nestingModel == HYFLOW_NESTING_OPEN) {
		if (!parentContext) {	// Top context commit here
			LOG_DEBUG("DTL :Top context Check Parent not throwing exception\n");
			return false;
		} else {
			// If not the top, throw exception only if parent is set aborted
			if (parentContext->getStatus() != TXN_ABORTED ) {
				// Inner transaction abort count
				increaseInnerAbortCount();
				int aborts = getInnerAbortCount();
				if ( aborts > 5 ) {
					LOG_DEBUG("DTL :Check Parent open repeated aborts throwing exception\n");
					resetInnerAbortCount();
					return true;
				}else {
					LOG_DEBUG("DTL :Check Parent open parent fine, not throwing exception\n");
					return false;
				}
			} else {
				LOG_DEBUG("DTL :Check Parent throwing exception\n");
				return true;
			}
		}
	}else if (nestingModel == HYFLOW_INTERNAL_OPEN) {
		// We never throw exception in Internal open, we just keep retrying
		return false;
	}else {
		Logger::fatal("DTL :Check Parent :Invalid Nesting Model\n");
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
bool DTL2Context::fetchObject(std::string id, bool isRead=true, bool abortOnNull=true) {
	/*
	 * Perform object availability check in Transaction itself
	 */
	// First of all check is object is part of delete set, Txn might have deleted
	LOG_DEBUG("DTL :FetchObject readSet size %u\n", readMap.size());
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cdi = deleteMap.find(id);
	if ( cdi != deleteMap.end()) {
		if (abortOnNull) {
			LOG_DEBUG("DTL :Object %s in Delete Map, abort this transaction\n", id.c_str());
			status = TXN_ABORTED;
			TransactionException objectDeleted("DTL :Object in delete map, abort this transaction\n");
			throw objectDeleted;
		}else {
			LOG_DEBUG("DTL :Object %s was Deleted by this transaction\n", id.c_str());
			return false;
		}
	}

	// check if object is already part of read set, all objects are added to read set
	// which are either fetched for read or write, therefore need not to check for write set
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cri = readMap.find(id);
	if ( cri != readMap.end()) {
		// We already have object in transactions read set nothing to do return
		LOG_DEBUG("DTL :Object %s already available in ReadSet\n", id.c_str());
		return true;
	}

	// Search in publish set too, user may lose its own created objects
	// We need not to test to parent transactions first as transaction create unique objects
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cpi = publishMap.find(id);
	if ( cpi != publishMap.end() ) {
		LOG_DEBUG("DTL :Object %s found in transaction publish Set\n", id.c_str());
		// Copy object to its read set
		HyflowObject* prCopy = NULL;
		cpi->second->getClone(&prCopy);

		readMap[id] = prCopy;
		if (!isRead) {
			HyflowObject* pwCopy = NULL;
			cpi->second->getClone(&pwCopy);
			writeMap[id] = pwCopy;
		}
		return true;
	}

	/*
	 * Perform object availability check in Transaction parents, Useful for close nesting
	 */
	for (DTL2Context* current= (DTL2Context*)parentContext; current != NULL; current = (DTL2Context*)current->parentContext) {
		{
			// Check if object got Deleted by any of previous inner transaction
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pdi = current->deleteMap.find(id);
			if ( pdi != current->deleteMap.end()) {
				if (abortOnNull) {
					LOG_DEBUG("DTL :Object %s in parent Delete Map, abort this transaction\n", id.c_str());
					status = TXN_ABORTED;
					TransactionException objectDeleted("DTL :Object in delete map, abort this transaction\n");
					throw objectDeleted;
				}else {
					LOG_DEBUG("DTL :Object %s got Deleted by parent\n", id.c_str());
					return false;
				}
			}
		}

		{
			// Check if object got updated by any of previous inner transaction: InnerTxn should read updated object
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pwi = current->writeMap.find(id);
			if ( pwi != current->writeMap.end()) {
				// We got object in parent transaction
				LOG_DEBUG("Got %s in parent transaction writeMap\n", id.c_str());
				HyflowObject* prCopy = NULL;
				pwi->second->getClone(&prCopy);

				readMap[id] = prCopy;
				if (!isRead) {
					HyflowObject* pwCopy = NULL;
					pwi->second->getClone(&pwCopy);
					writeMap[id] = pwCopy;
	 			}
				return true;
			}
		}

		{
			// Check if object exist in Read Map
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pri = current->readMap.find(id);
			if (pri != current->readMap.end()) {
				// We got object in parent transaction
				LOG_DEBUG("Got %s in parent transaction readMap\n", id.c_str());
				HyflowObject* prCopy = NULL;
				pri->second->getClone(&prCopy);
				readMap[id] = prCopy;
				if (!isRead) {
					HyflowObject* pwCopy = NULL;
					pri->second->getClone(&pwCopy);
					writeMap[id] = pwCopy;
				}
				return true;
			}
		}

		{
			// Check if object exist in publish set
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ppubItr = current->publishMap.find(id);
			if (ppubItr != current->publishMap.end()) {
				// We got object in parent transaction
				LOG_DEBUG("Got %s in parent transaction publishMap\n", id.c_str());
				HyflowObject* prCopy = NULL;
				ppubItr->second->getClone(&prCopy);

				readMap[id] = prCopy;
				if (!isRead) {
					HyflowObject* pwCopy = NULL;
					ppubItr->second->getClone(&pwCopy);
					writeMap[id] = pwCopy;
				}
				return true;
			}
		}
	}

	// Go over network and fetch the object
	HyflowObject* obj = DirectoryManager::locate(id, true, txnId);
	// If object got deleted then throw abort exception
	if (obj == NULL) {
		if (abortOnNull) {
			LOG_DEBUG("DTL :Object %s got Deleted, need to abort this transaction\n", id.c_str());
			status = TXN_ABORTED;
			TransactionException objectDeleted("DTL :Object got Deleted, need to delete this transaction\n");
			throw objectDeleted;
		}else {
			LOG_DEBUG("DTL :Object %s got Deleted\n", id.c_str());
			return false;
		}
	}
	// Perform early validation step, always use highestSendClock, it is update by objectAccessMessage
	// First do forwarding on current readSet then add new object to it
	forward(highestSenderClock);
	int checkPointIndex = CheckPointProvider::getCheckPointIndex();
	obj->setAccessCheckPoint(checkPointIndex);
	LOG_DEBUG("DTL :Fetched object %s at checkPointIndex %d\n", obj->getId().c_str(), checkPointIndex);
	if (isRead) {
		LOG_DEBUG("DTL :Adding fetched object %s to readSet\n", id.c_str());
		readMap[id] = obj;
	}else {
		// Save a reference of object, so we don't have to go to network get non-manipulated copy
		HyflowObject* referenceCopy = NULL;
		obj->getClone(&referenceCopy);
		readMap[id] = referenceCopy;
		LOG_DEBUG("DTL :Adding fetched object %s to writeSet\n", id.c_str());
		writeMap[id] = obj;
	}
	return true;
}

void DTL2Context::fetchObjects(std::string ids[], int objCount, bool isRead=true) {
	for(int i=0; i < objCount; i++) {
		fetchObject(ids[i], isRead);
	}
}

HyflowObject* DTL2Context::locateObject(std::string id, bool abortOnNull) {
	/*
	 * Perform object availability check in Transaction itself
	 */
	// First of all check is object is part of delete set, Txn might have deleted
	LOG_DEBUG("DTL :FetchObject readSet size %u\n", readMap.size());
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cdi = deleteMap.find(id);
	if ( cdi != deleteMap.end()) {
		if (abortOnNull) {
			LOG_DEBUG("DTL :Object %s in Delete Map, abort this transaction\n", id.c_str());
			status = TXN_ABORTED;
			TransactionException objectDeleted("DTL :Object in delete map, abort this transaction\n");
			throw objectDeleted;
		}else {
			return NULL;
		}
	}

	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cwi = writeMap.find(id);
	if ( cwi != writeMap.end()) {
		return writeMap[id];
	}

	// check if object is already part of read set, all objects are added to read set
	// which are either fetched for read or write, therefore need not to check for write set
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cri = readMap.find(id);
	if ( cri != readMap.end()) {
		return readMap[id];
	}

	// Search in publish set too, user may lose its own created objects
	// We need not to test to parent transactions first as transaction create unique objects
	std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator cpi = publishMap.find(id);
	if ( cpi != publishMap.end() ) {
		LOG_DEBUG("DTL :Object %s found in transaction publish Set\n", id.c_str());
		// Copy object
		HyflowObject* prCopy = NULL;
		cpi->second->getClone(&prCopy);
		return prCopy;
	}

	/*
	 * Perform object availability check in Transaction parents, Useful for close and open nesting
	 */
	for (DTL2Context* current= (DTL2Context*)parentContext; current != NULL; current = (DTL2Context*)current->parentContext) {
		{
			// Check if object got Deleted by any of previous inner transaction
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pdi = current->deleteMap.find(id);
			if ( pdi != current->deleteMap.end()) {
				if (abortOnNull) {
					LOG_DEBUG("DTL :Object %s in parent Delete Map, abort this transaction\n", id.c_str());
					status = TXN_ABORTED;
					TransactionException objectDeleted("DTL :Object in delete map, abort this transaction\n");
					throw objectDeleted;
				}else {
					return NULL;
				}
			}
		}

		{
			// Check if object got updated by any of previous inner transaction: InnerTxn should read updated object
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pwi = current->writeMap.find(id);
			if ( pwi != current->writeMap.end()) {
				// We got object in parent transaction
				LOG_DEBUG("Got %s in parent transaction writeMap\n", id.c_str());
				HyflowObject* pwCopy = NULL;
				pwi->second->getClone(&pwCopy);
				return pwCopy;
			}
		}

		{
			// Check if object exist in Read Map
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator pri = current->readMap.find(id);
			if (pri != current->readMap.end()) {
				// We got object in parent transaction
				LOG_DEBUG("Got %s in parent transaction readMap\n", id.c_str());
				HyflowObject* prCopy = NULL;
				pri->second->getClone(&prCopy);
				return prCopy;
			}
		}

		{
			// Check if object exist in publish set
			std::map<std::string, HyflowObject*, ObjectIdComparator>::iterator ppubItr = current->publishMap.find(id);
			if (ppubItr != current->publishMap.end()) {
				// We got object in parent transaction
				LOG_DEBUG("Got %s in parent transaction publishMap\n", id.c_str());
				HyflowObject* ppCopy = NULL;
				ppubItr->second->getClone(&ppCopy);
				return ppCopy;
			}
		}
	}

	// Go over network and fetch the object
	HyflowObject* obj = DirectoryManager::locate(id, true, txnId);
	// If object got deleted then throw abort exception
	if (obj == NULL) {
		if (abortOnNull) {
			LOG_DEBUG("DTL :Object %s got Deleted, need to abort this transaction\n", id.c_str());
			status = TXN_ABORTED;
			TransactionException objectDeleted("DTL :Object got Deleted, need to delete this transaction\n");
			throw objectDeleted;
		}else {
			return NULL;
		}
	}

	// Perform early validation step, always use highestSendClock, it is update by objectAccessMessage
	// First do forwarding on current readSet then add new object to it
	forward(highestSenderClock);
	int checkPointIndex = CheckPointProvider::getCheckPointIndex();
	obj->setAccessCheckPoint(checkPointIndex);
	LOG_DEBUG("DTL :Fetched object %s at checkPointIndex %d\n", obj->getId().c_str(), checkPointIndex);
	return obj;
}

bool DTL2Context::haveAbstractLocks() {
	if (abstractReadLocks.empty() && abstractWriteLocks.empty()) {
		return false;
	}
	return true;
}


void DTL2Context::increaseInnerAbortCount() {
	if (!innerAbortCount.get()) {
		HyInteger* aborts = new HyInteger(0);
		innerAbortCount.reset(aborts);
	}
	innerAbortCount.get()->increaseValue();
}

int DTL2Context::getInnerAbortCount() {
	if (!innerAbortCount.get()) {
		HyInteger* aborts = new HyInteger(0);
		innerAbortCount.reset(aborts);
	}
	return innerAbortCount.get()->getValue();
}

void DTL2Context::resetInnerAbortCount() {
	if (!innerAbortCount.get()) {
		HyInteger* aborts = new HyInteger(0);
		innerAbortCount.reset(aborts);
	}
	innerAbortCount.get()->setValue(0);
}


} /* namespace vt_dstm */
