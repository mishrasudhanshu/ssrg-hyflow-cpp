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
	if(vt_dstm::CheckPointProvider::isCheckPointingEnabled()) {\
		vt_dstm::CheckPointProvider::checkPointInit();\
		ucontext_t* checkPoint = new ucontext_t(); \
		vt_dstm::CheckPointProvider::saveCheckPoint(checkPoint); \
		getcontext(checkPoint); \
	}\
}while(0)\

#define HYFLOW_CHECKPOINT_HERE \
{\
	if(vt_dstm::CheckPointProvider::isCheckPointingEnabled()) {\
		vt_dstm::CheckPointProvider::increaseCheckPointIndex();\
		ucontext_t* checkPoint = new ucontext_t(); \
		vt_dstm::CheckPointProvider::saveCheckPoint(checkPoint); \
		getcontext(checkPoint); \
		LOG_DEBUG("Restoring the CheckPoint\n");\
	}\
}\

#define HYFLOW_STORE(VAR_REFERENCE, VAR_VALUE) \
do {\
	\
}while(0)\

#define HYFLOW_RESTORE_ALL \
do {\
\
}while(0)\

namespace vt_dstm {

class CheckPointProvider {
public:
	// TODO: Add all primary data types
	static void storeUserValue(int *Reference, int *value);
	static void storeUserValue(std::string *Reference, std::string *Referece);
	static void storeUserValue(void **Reference, void **value);

	static std::vector<std::vector<std::pair<int*, int*> > >** threadIntegerStore;
	static std::vector<std::vector<std::pair<std::string*, std::string*> > >** threadStringStore;
	static std::vector<std::vector<std::pair<void**, void**> > >** threadPointerStore;

	static volatile bool* isTransactionComplete;
	static volatile int* checkPointIndex;
	static std::vector<ucontext_t *>** checkPoints;
	static bool checkPointingEnabled;

	static bool isCheckPointingEnabled(){
		return checkPointingEnabled;
	}

	static int getCheckPointIndex();
	void setCheckPointIndex(int checkPointIndex);

	CheckPointProvider();
	virtual ~CheckPointProvider();

	static void checkPointProviderInit();
	static void checkPointInit();

	static void increaseCheckPointIndex();
	static void setThreadCheckPointIndex(int checkPointIndex);
	static void saveCheckPoint(ucontext_t* checkPoint);
	static void startCheckPoint(int checkPointIndex);
	static void test();
};



} /* namespace vt_dstm */

#endif /* CHECKPOINTPROVIDER_H_ */
