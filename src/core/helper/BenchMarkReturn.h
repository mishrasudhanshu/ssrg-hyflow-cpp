/*
 * BenchMarkReturn.h
 *
 *  Created on: Dec 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BENCHMARKRETURN_H_
#define BENCHMARKRETURN_H_

namespace vt_dstm {

class BenchMarkReturn {
protected:
	bool onHeap;
public:
	BenchMarkReturn() {onHeap = false;}
	virtual ~BenchMarkReturn() {}

	virtual void getClone(BenchMarkReturn** bmr) = 0;

	bool isOnHeap() const {
		return onHeap;
	}

	void setOnHeap(bool onHeap) {
		this->onHeap = onHeap;
	}
};

} /* namespace vt_dstm */

#endif /* BENCHMARKRETURN_H_ */
