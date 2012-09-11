/*
 * RandomIdProvider.h
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef RANDOMIDPROVIDER_H_
#define RANDOMIDPROVIDER_H_

#include <cstdlib>
#include <time.h>

namespace vt_dstm {

class RandomIdProvider {
	int objectCount;
	int tmpCount;
	int *valueArray;
public:
	RandomIdProvider(int objectCount) {
		this->objectCount = objectCount;
		this->tmpCount = 0;
		valueArray = new int[objectCount];
		for (int i=0;i <objectCount ; i++) {
			valueArray[i] = i;
		}
		std::srand(std::time(NULL));
	}

	virtual ~RandomIdProvider() {delete[] valueArray;};

	int getNext() {
		if (tmpCount < objectCount) {
			int pos = tmpCount + (rand() % (objectCount -tmpCount));
			int value  = valueArray[pos];
			int tmp = valueArray[tmpCount];
			valueArray[pos] = tmp;
			tmpCount++;
			return value;
		} else {
			throw "Asking more values than object Count!!";
		}
		return -1;
	}

	static int getRandomNumber() {
		return rand();
	}
};

} /* namespace vt_dstm */

#endif /* RANDOMIDPROVIDER_H_ */
