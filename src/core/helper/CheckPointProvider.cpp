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
#include "../../benchMarks/BenchmarkExecutor.h"

namespace vt_dstm {

volatile int* CheckPointProvider::jumpCheck = NULL;

volatile bool* CheckPointProvider::isTransactionComplete = NULL;
std::vector<ucontext_t *>** CheckPointProvider::checkPoints = NULL;
volatile int* CheckPointProvider::checkPointIndex = NULL;
bool CheckPointProvider::checkPointingEnabled = false;

std::map<int, std::vector<std::pair<int*, int> > >** CheckPointProvider::threadIntegerStore = NULL;
std::map<int, std::vector<std::pair<std::string*, std::string> > >** CheckPointProvider::threadStringStore = NULL;
std::map<int, std::vector<std::pair<void**, void*> > >** CheckPointProvider::threadPointerStore = NULL;

CheckPointProvider::CheckPointProvider() {}

CheckPointProvider::~CheckPointProvider() {}

bool CheckPointProvider::isCheckPointingEnabled(){
	if (ThreadMeta::getThreadId() == NetworkManager::getThreadCount()) {
		return false;
	}
	return checkPointingEnabled;
}


// TODO: Add some way to free up memory, currently just leaking
void CheckPointProvider::checkPointProviderInit() {
	if (!checkPointingEnabled) {
		int threadCount = NetworkManager::getThreadCount();
		jumpCheck = new volatile int[threadCount];
		isTransactionComplete = new volatile bool[threadCount] ;
		checkPointIndex = new volatile int[threadCount];
		checkPoints = new std::vector<ucontext_t*>*[threadCount];

		threadIntegerStore = new std::map<int, std::vector<std::pair<int*, int> > >*[threadCount];
		threadStringStore = new std::map<int, std::vector<std::pair<std::string*, std::string> > >*[threadCount];
		threadPointerStore = new std::map<int, std::vector<std::pair<void**, void*> > >*[threadCount];

		for (int i=0; i<threadCount ; i++) {
			checkPoints[i] = new std::vector<ucontext_t *>();
			threadIntegerStore[i] = new std::map<int, std::vector<std::pair<int*, int> > >();
			threadStringStore[i] = new std::map<int, std::vector<std::pair<std::string*, std::string> > >();
			threadPointerStore[i] = new std::map<int, std::vector<std::pair<void**, void*> > >();
		}
	}
	checkPointingEnabled = true;
}

void CheckPointProvider::checkPointInit() {
	if (isCheckPointingEnabled()) {
		int threadId = ThreadMeta::getThreadId();
		isTransactionComplete[threadId] = false;
		checkPointIndex[threadId] = 0;
		jumpCheck[threadId] = 0;
		for (unsigned int i=0 ; i<checkPoints[threadId]->size() ; i++ ) {
			delete checkPoints[threadId]->at(i);
		}
		checkPoints[threadId]->clear();
		threadIntegerStore[threadId]->clear();
		threadStringStore[threadId]->clear();
		threadPointerStore[threadId]->clear();
	}
}

void CheckPointProvider::increaseCheckPointIndex() {
	if (isCheckPointingEnabled()) {
		checkPointIndex[ThreadMeta::getThreadId()]++;
	}
}

void CheckPointProvider::storeUserValue(int *Reference, int value) {
	unsigned int threadId = ThreadMeta::getThreadId();
	int nextCheckPoint = getCheckPointIndex()+1;

	std::map<int, std::vector<std::pair<int*, int> > >& intValueMap = *(threadIntegerStore[threadId]);
	std::map<int, std::vector<std::pair<int*, int> > >::iterator itr;
	itr = intValueMap.find(nextCheckPoint);

	if (itr == intValueMap.end()) {
		std::vector<std::pair<int*, int> > checkPointIntPairs;
		intValueMap[nextCheckPoint] = checkPointIntPairs;
	}
	std::pair<int*,int> intPair(Reference, value);
	intValueMap.at(nextCheckPoint).push_back(intPair);
}

void CheckPointProvider::storeUserValue(std::string *Reference, std::string value) {
	unsigned int threadId = ThreadMeta::getThreadId();
	int nextCheckPoint = getCheckPointIndex()+1;

	std::map<int, std::vector<std::pair<std::string*, std::string> > >& strValueMap = *(threadStringStore[threadId]);
	std::map<int, std::vector<std::pair<std::string*, std::string> > >::iterator itr;
	itr = strValueMap.find(nextCheckPoint);

	if (itr == strValueMap.end()) {
		std::vector<std::pair<std::string*, std::string> > checkPointIntPairs;
		strValueMap[nextCheckPoint] = checkPointIntPairs;
	}

	std::pair<std::string*,std::string> stringPair(Reference, value);
	strValueMap.at(nextCheckPoint).push_back(stringPair);
}

void CheckPointProvider::storeUserValue(void **Reference, void *value) {
	unsigned int threadId = ThreadMeta::getThreadId();
	int nextCheckPoint = getCheckPointIndex()+1;

	std::map<int, std::vector<std::pair<void**, void*> > >& pointerValueMap = *(threadPointerStore[threadId]);
	std::map<int, std::vector<std::pair<void**, void*> > >::iterator itr;
	itr = pointerValueMap.find(nextCheckPoint);

	if (itr == pointerValueMap.end()) {
		std::vector<std::pair<void**, void*> > checkPointIntPairs;
		pointerValueMap[nextCheckPoint] = checkPointIntPairs;
	}
	std::pair<void**,void*> voidPair(Reference, value);
	pointerValueMap.at(nextCheckPoint).push_back(voidPair);
}

int CheckPointProvider::getCheckPointIndex() {
	if (isCheckPointingEnabled()) {
		return checkPointIndex[ThreadMeta::getThreadId()];
	}
	return 0;
}

void CheckPointProvider::setCheckPointIndex(int cpi) {
	if (isCheckPointingEnabled()) {
		checkPointIndex[ThreadMeta::getThreadId()] = cpi;
		LOG_DEBUG("CPP :PreJump CheckPoint Index %d\n", cpi);
	}
}

void CheckPointProvider::startCheckPoint(int checkPointIndex) {
	if (isCheckPointingEnabled()) {
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
				cleanUpUserValues(i);
			}else if(checkPointIndex == i) {
				LOG_DEBUG("CPP : Found checkpoint %d\n", i);
				restartPoint = checkPoints[threadId]->at(i);
				setCheckPointIndex(i);
				break;
			} else {
				Logger::fatal("CPP : CheckPoint loop should have broken out\n");
			}
		}

		BenchmarkExecutor::increaseMetaData(HYFLOW_METADATA_CHECKPOINT_RESUME);
		LOG_DEBUG("CPP : Setting checkPoint Jump Point %d\n", checkPointIndex);
		jumpCheck[threadId] = checkPointIndex;
		setcontext(restartPoint);
	}
}

void CheckPointProvider::cleanUpUserValues(int forCheckPoint) {
	int threadId = ThreadMeta::getThreadId();
	threadIntegerStore[threadId]->erase(forCheckPoint);
	threadStringStore[threadId]->erase(forCheckPoint);
	threadPointerStore[threadId]->erase(forCheckPoint);
}


void CheckPointProvider::restoreUserValues() {
	int threadId = ThreadMeta::getThreadId();
	int whereWeJumped = jumpCheck[threadId];
	jumpCheck[threadId] = 0;

	if ( whereWeJumped ) {
		LOG_DEBUG("CPP : Restoring User values\n");
		std::map<int, std::vector<std::pair<int*, int> > > & intMap = *(threadIntegerStore[threadId]);
		std::map<int, std::vector<std::pair<int*, int> > >::iterator intMapItr =  intMap.find(whereWeJumped);
		if (intMapItr != intMap.end()) {
			std::vector<std::pair<int*, int> >& intStore = (*intMapItr).second;
			std::vector<std::pair<int*, int> >::iterator iitr;
			for ( iitr = intStore.begin() ; iitr < intStore.end() ; iitr++ ) {
				LOG_DEBUG("CPP : Restoring Integer Value from %d to %d\n", *(iitr->first), iitr->second);
				*(iitr->first) = iitr->second;
			}
		}

		std::map<int, std::vector<std::pair<std::string*, std::string> > >& strMap = *(threadStringStore[threadId]);
		std::map<int, std::vector<std::pair<std::string*, std::string> > >::iterator strMapItr = strMap.find(whereWeJumped);
		if (strMapItr != strMap.end()) {
			std::vector<std::pair<std::string*, std::string> >& strStore = (*strMapItr).second;
			std::vector<std::pair<std::string*, std::string> >::iterator sitr;
			for ( sitr = strStore.begin(); sitr < strStore.end() ; sitr++ ) {
				LOG_DEBUG("CPP : Restoring String Value\n");
				*(sitr->first) = sitr->second;
			}
		}

		std::map<int, std::vector<std::pair<void**, void*> > >& pointerMap = *(threadPointerStore[threadId]);
		std::map<int, std::vector<std::pair<void**, void*> > >::iterator pointerMapItr = pointerMap.find(whereWeJumped);
		if (pointerMapItr != pointerMap.end()) {
			std::vector<std::pair<void**, void*> >& pointerStore = (*pointerMapItr).second;
			std::vector<std::pair<void**, void*> >::iterator vitr;
			for ( vitr = pointerStore.begin() ; vitr < pointerStore.end(); vitr++ ) {
				LOG_DEBUG("CPP : Restoring Pointer Value\n");
				*(vitr->first) = vitr->second;
			}
		}
		LOG_DEBUG("CPP :PostJump CheckPoint Index %d\n", whereWeJumped);
	}
}

void CheckPointProvider::saveCheckPoint(ucontext_t* checkPoint) {
	if (isCheckPointingEnabled()) {
		int threadId = ThreadMeta::getThreadId();
		LOG_DEBUG("CPP : Saved checkpoint number %d\n", checkPoints[threadId]->size());
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

