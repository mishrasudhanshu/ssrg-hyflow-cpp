/*
 * RandomIdProvider.cpp
 *
 *  Created on: Dec 29, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdlib>
#include <climits>

#include "RandomIdProvider.h"
#include "../../util/logging/Logger.h"

namespace vt_dstm {

RandomIdProvider::RandomIdProvider(int objectCount) {
	this->objectCount = objectCount;
	this->tmpCount = 0;
	valueArray = new int[objectCount];
	for (int i=0;i <objectCount ; i++) {
		valueArray[i] = i;
	}
}

RandomIdProvider::~RandomIdProvider() {delete[] valueArray;};

int RandomIdProvider::getNext() {
	if (tmpCount < objectCount) {
		std::srand(getSeed());
		int pos = tmpCount + (rand() % (objectCount -tmpCount));
		int value  = valueArray[pos];
		int tmp = valueArray[tmpCount];
		valueArray[pos] = tmp;
		tmpCount++;
		return value;
	} else {
		Logger::fatal("Asking %d values than object Count = %d!!\n", tmpCount, objectCount);
		throw "Asking more values than object Count!!";
	}
	return -1;
}

unsigned int RandomIdProvider::getSeed() {
	unsigned long long t = Logger::getCurrentMicroSec();
	return (t%INT_MAX -1);
}

int RandomIdProvider::getRandomNumber() {
	std::srand(getSeed());
	return rand();
}

}/* namespace vt_dstm */
