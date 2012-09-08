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
#include "../../helper/ObjectIdComparator.h"
#include "../HyflowContext.h"

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
	 * List of write locks hold by transaction
	 */
	std::set<std::string> lockSet;
	int tnxClock;
	int highestSenderClock;

	bool lockObject(HyflowObject *obj);
	void unlockObjectOnFail(HyflowObject *obj);
	void unlockObject(HyflowObject *obj);
	bool validateObject(HyflowObject* obj);
public:
	DTL2Context();
	virtual ~DTL2Context();

	void forward(int senderClock);
	void beforeReadAccess(HyflowObject *obj);
	HyflowObject* onReadAccess(HyflowObject *obj);
	HyflowObject* onWriteAccess(HyflowObject *obj);
	void commit();
	void abort();
	void updateClock(int c);
};

} /* namespace vt_dstm */

#endif /* DTL2CONTEXT_H_ */
