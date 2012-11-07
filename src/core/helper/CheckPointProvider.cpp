/*
 * CheckPointProvider.cpp
 *
 *  Created on: Nov 3, 2012
 *      Author: mishras[at]vt.edu
 */
#include <cstdio>
#include <cstdlib>

#include "CheckPointProvider.h"
#include "../../util/networking/NetworkManager.h"
#include "../../util/concurrent/ThreadMeta.h"
#include "../../util/logging/Logger.h"

namespace vt_dstm {
volatile bool* CheckPointProvider::isTransactionComplete = NULL;
std::vector<ucontext_t *>** CheckPointProvider::checkPoints = NULL;
volatile int* CheckPointProvider::checkPointIndex = NULL;
bool CheckPointProvider::checkPointingEnabled = false;

CheckPointProvider::CheckPointProvider() {}

CheckPointProvider::~CheckPointProvider() {}

// TODO: Add some way to free up memory, currently just leaking
void CheckPointProvider::checkPointProviderInit() {
	if (!checkPointingEnabled) {
		int threadCount = NetworkManager::getThreadCount();
		isTransactionComplete = new volatile bool[threadCount] ;
		checkPointIndex = new volatile int[threadCount];
		checkPoints = new std::vector<ucontext_t*>*[threadCount];
		for (int i=0; i<threadCount ; i++) {
			checkPoints[i] = new std::vector<ucontext_t *>();
		}
	}
	checkPointingEnabled = true;
}

void CheckPointProvider::checkPointInit() {
	if (checkPointingEnabled) {
		int threadId = ThreadMeta::getThreadId();
		isTransactionComplete[threadId] = false;
		checkPointIndex[threadId] = 0;
		for (unsigned int i=0 ; i<checkPoints[threadId]->size() ; i++ )
			delete checkPoints[threadId]->at(i);
		checkPoints[threadId]->clear();
	}
}

void CheckPointProvider::increaseCheckPointIndex() {
	if (checkPointingEnabled) {
		checkPointIndex[ThreadMeta::getThreadId()]++;
	}
}

int CheckPointProvider::getCheckPointIndex() {
	if (checkPointingEnabled) {
		return checkPointIndex[ThreadMeta::getThreadId()];
	}
	return 0;
}
void CheckPointProvider::setCheckPointIndex(int cpi) {
	if (checkPointingEnabled) {
		checkPointIndex[ThreadMeta::getThreadId()] = cpi;
	}
}

void CheckPointProvider::startCheckPoint(int checkPointIndex) {
	if (checkPointingEnabled) {
		int threadId = ThreadMeta::getThreadId();
		if (checkPointIndex > (checkPoints[threadId]->size()-1)) {
			Logger::fatal("CPP : Beyond checkpoint\n");
			return;
		}

		ucontext_t * restartPoint = NULL;
		for (int i=checkPoints[threadId]->size()-1 ; i >= 0; i--) {
			if(checkPointIndex < i) {
				restartPoint = checkPoints[threadId]->at(i);
				checkPoints[threadId]->pop_back();
				delete restartPoint;
				LOG_DEBUG("CPP : Deleted checkpoint %d\n", i);
				restartPoint = NULL;
			}else if(checkPointIndex == i) {
				restartPoint = checkPoints[threadId]->at(i);
				break;
			} else {
				Logger::fatal("CPP : CheckPoint loop should have broken out\n");
			}
		}

		LOG_DEBUG("CPP : Restarting checkPoint\n");
		setcontext(restartPoint);
	}
}

void CheckPointProvider::saveCheckPoint(ucontext_t* checkPoint) {
	if (checkPointingEnabled) {
		int threadId = ThreadMeta::getThreadId();
		checkPoints[threadId]->push_back(checkPoint);
	}
}

void CheckPointProvider::test(){
	printf("Testing checkPointProvider\n\n\n");

	HYFLOW_CHECKPOINT_INIT;

	printf("Working till CP1\n");

	HYFLOW_CHECKPOINT_HERE;

	printf("Working till CP2\n");

	HYFLOW_CHECKPOINT_HERE;

	printf("Working till CP3\n");
	HYFLOW_CHECKPOINT_HERE;

	printf("Working till end\n");
	int cpSuccess;
	scanf("%d", &cpSuccess);
	CheckPointProvider::startCheckPoint(cpSuccess);
	printf("Die...\n");
}

} /* namespace vt_dstm */

