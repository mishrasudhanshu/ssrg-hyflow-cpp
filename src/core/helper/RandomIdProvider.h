/*
 * RandomIdProvider.h
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef RANDOMIDPROVIDER_H_
#define RANDOMIDPROVIDER_H_

namespace vt_dstm {

class RandomIdProvider {
	int objectCount;
	int tmpCount;
	int *valueArray;

public:
	RandomIdProvider(int objectCount);

	virtual ~RandomIdProvider();

	int getNext();

	static int getRandomNumber();

	static unsigned int getSeed();
};

} /* namespace vt_dstm */

#endif /* RANDOMIDPROVIDER_H_ */
