/*
 * HyflowContextFactory.h
 *
 *  Created on: Oct 29, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWCONTEXTFACTORY_H_
#define HYFLOWCONTEXTFACTORY_H_

#include <vector>
#include "HyflowContext.h"

namespace vt_dstm {

class HyflowContextFactory {
	std::vector<HyflowContext*> contextStack;
	int contextStackIndex;
	/*
	 * Required to create unique txnId
	 */
	int txnIndex;
	HyflowContext* getContextFromStack();
	HyflowContext* getFreshContext();
	HyflowContext* getFreshContext(Hyflow_NestingModel nm);
public:
	HyflowContextFactory();
	virtual ~HyflowContextFactory();

	bool isContextInit();
	HyflowContext* getContextInstance();
	HyflowContext* getContextInstance(Hyflow_NestingModel nm);
	void releaseContextInstance(Hyflow_NestingModel nm);

	int getTxnIndex() const {
		return txnIndex;
	}
};

} /* namespace vt_dstm */

#endif /* HYFLOWCONTEXTFACTORY_H_ */
