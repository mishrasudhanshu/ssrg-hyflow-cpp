/*
 * dtl2Context.h
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DTL2CONTEXT_H_
#define DTL2CONTEXT_H_

#include <set>
#include <map>
#include <string>
#include <boost/thread/tss.hpp>
#include "../../helper/ObjectIdComparator.h"
#include "../HyflowContext.h"
#include "../../../util/concurrent/HyInteger.h"
#include "../AbstractLock.h"
#include "../../helper/Atomic.h"

namespace vt_dstm {

class DTL2Context: public vt_dstm::HyflowContext {
	/**
	 * Read maps to hold objects currently read by transaction
	 */
	std::map<std::string, HyflowObject*, ObjectIdComparator> readMap;
	/**
	 * Write maps to hold objects currently read by transaction
	 */
	std::map<std::string, HyflowObject*, ObjectIdComparator> writeMap;
	/**
	 * List of object created in transaction
	 */
	std::map<std::string, HyflowObject*, ObjectIdComparator> publishMap;
	/**
	 * List of objects deleted in transaction
	 */
	std::map<std::string, HyflowObject*, ObjectIdComparator> deleteMap;
	/**
	 * TODO: Remove :List of write locks hold by transaction
	 */
	std::set<std::string> lockSet;
	/*
	 * Abstract read Locks owned by transaction
	 */
	std::map<std::string, AbstractLock*> abstractReadLocks;
	/*
	 * Abstract write Locks owned by transaction
	 */
	std::map<std::string, AbstractLock*> abstractWriteLocks;

	/*
	 * Actions on abort or on commit
	 */
	std::vector<Atomic*> actionList;
	void* currentAction;
	int tnxClock;
	int highestSenderClock;

	static boost::thread_specific_ptr<HyInteger> innerAbortCount;
	bool isWrite;
	bool lockObject(HyflowObject *obj);
	void unlockObjectOnFail(HyflowObject *obj);
	void unlockObject(HyflowObject *obj);
	bool validateObject(HyflowObject* obj);
	void validateObjectCP(HyflowObject* obj);
	void tryCommit();
	void tryCommitCP();
	void tryCommitON();
	void reallyCommit();
	void reallyCommitON();
	void cleanAllMaps();
	void mergeIntoParents();
	void cleanSetTillCheckPoint(int checkPoint);
	void contextReset();
	bool doAbstractLock(AbstractLock* absLock, bool read);
	void doAbstractUnLock(AbstractLock* absLock, bool read);
	unsigned long long getRootTxnId();
public:
	DTL2Context();
	DTL2Context(Hyflow_NestingModel nm);
	virtual ~DTL2Context();

	void contextInit();
	void contextDeinit();
	void forward(int senderClock);
	void beforeReadAccess(HyflowObject *obj);
	const HyflowObject* onReadAccess(HyflowObject *obj);
	const HyflowObject* onReadAccess(std::string id);
	HyflowObject* onWriteAccess(HyflowObject *obj);
	HyflowObject* onWriteAccess(std::string id);
	void addToPublish(HyflowObject* newObject);
	void addToDelete(HyflowObject* deleteObject);
	void commit();
	void rollback();
	bool checkParent();
	void updateClock(int c);
	bool fetchObject(std::string id, bool isRead, bool abortOnNull);
	void fetchObjects(std::string ids[], int objCount, bool isRead);
	HyflowObject* locateObject(std::string objId, bool abortOnNull);
	static void increaseInnerAbortCount();
	static int getInnerAbortCount();
	static void resetInnerAbortCount();
	void onLockAccess(std::string benchObject, std::string lockname, bool readlock);
	void setCurrentAction(void* currentAction);
	void addAbstractLock(std::string lockName, void* abstractLock, bool read);
	virtual void addAction(void* action);

	bool isIsWrite() const {
		return isWrite;
	}
};

} /* namespace vt_dstm */

#endif /* DTL2CONTEXT_H_ */
