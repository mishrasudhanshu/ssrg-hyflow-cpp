/*
 * CheckPointProvider.cpp
 *
 *  Created on: Nov 3, 2012
 *      Author: mishras[at]vt.edu
 */
#include <stdio.h>
#include <stdlib.h>

#include "CheckPointProvider.h"

namespace vt_dstm {
volatile bool CheckPointProvider::isTransactionComplete = false;
std::vector<ucontext_t *> CheckPointProvider::checkPoints;
volatile int CheckPointProvider::checkPointIndex = -1;

CheckPointProvider::CheckPointProvider() {}

CheckPointProvider::~CheckPointProvider() {}

void CheckPointProvider::startCheckPoint(int checkPointIndex) {
	if (checkPointIndex > (checkPoints.size()-1)) {
		// Log error and throw exception
		printf("Beyond checkpoint\n");
		return;
	}

	ucontext_t * restartPoint = NULL;
	for (int i=checkPoints.size()-1 ; i >= 0; i--) {
		if(checkPointIndex < i) {
			restartPoint = checkPoints[i];
			checkPoints.pop_back();
			delete restartPoint;
			printf("Deleted checkpoint %d\n", i);
			restartPoint = NULL;
		}else if(checkPointIndex == i) {
			restartPoint = checkPoints[i];
			break;
		} else {
			printf("Should have broken out\n");
		}
	}

	printf("Restarting checkPoint\n");
	setcontext(restartPoint);
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

