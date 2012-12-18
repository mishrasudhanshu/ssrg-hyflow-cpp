/*
 * BenchMarkArgs.h
 *
 *  Created on: Dec 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BENCHMARKARGS_H_
#define BENCHMARKARGS_H_

namespace vt_dstm {

class BenchMarkArgs {
public:
	BenchMarkArgs() {}
	virtual ~BenchMarkArgs() {}

	virtual void getClone(BenchMarkArgs** bmr) = 0;
};

} /* namespace vt_dstm */

#endif /* BENCHMARKARGS_H_ */
