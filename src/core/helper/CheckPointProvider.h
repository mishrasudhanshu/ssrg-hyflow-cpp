/*
 * CheckPointProvider.h
 *
 *  Created on: Nov 3, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CHECKPOINTPROVIDER_H_
#define CHECKPOINTPROVIDER_H_

#include <ucontext.h>
#include <vector>

#include "../context/ContextManager.h"

#define HYFLOW_CHECKPOINT_INIT \
do{\
	if(vt_dstm::ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {\
		vt_dstm::CheckPointProvider::isTransactionComplete = false;	\
		ucontext_t* checkPoint = new ucontext_t(); \
		vt_dstm::CheckPointProvider::checkPointIndex = 0;\
		vt_dstm::CheckPointProvider::checkPoints.push_back(checkPoint); \
		getcontext(checkPoint); \
	}\
}while(0)\

#define HYFLOW_CHECKPOINT_HERE \
{\
	if(vt_dstm::ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {\
		ucontext_t* checkPoint = new ucontext_t(); \
		vt_dstm::CheckPointProvider::checkPointIndex++;\
		vt_dstm::CheckPointProvider::checkPoints.push_back(checkPoint); \
		getcontext(checkPoint); \
	}\
}\

namespace vt_dstm {

class CheckPointProvider {
public:
	static volatile bool isTransactionComplete;
	static volatile int checkPointIndex;
	static std::vector<ucontext_t *> checkPoints;
	CheckPointProvider();
	virtual ~CheckPointProvider();

	static void startCheckPoint(int checkPointIndex);
	static void test();
};



} /* namespace vt_dstm */

#endif /* CHECKPOINTPROVIDER_H_ */
