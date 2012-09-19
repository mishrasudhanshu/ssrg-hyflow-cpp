/*
 * HyInteger.h
 *
 *  Created on: Sep 19, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYINTEGER_H_
#define HYINTEGER_H_

namespace vt_dstm {

class HyInteger {
	int value;
public:
	HyInteger() {}
	HyInteger (int v) {value =v;}
	~HyInteger() {}

	int getValue() const {
		return value;
	}

	void setValue(int value) {
		this->value = value;
	}

	void increaseValue() {
		value++;
	}
};

} /* namespace vt_dstm */

#endif /* HYINTEGER_H_ */
