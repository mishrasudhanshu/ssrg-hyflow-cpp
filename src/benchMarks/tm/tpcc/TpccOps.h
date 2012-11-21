/*
 * TPCCOps.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCOPS_H_
#define TPCCOPS_H_

namespace vt_dstm {

class TpccOps {
public:
	TpccOps();
	virtual ~TpccOps();

	static double newOrder();
	static void payment();
	static void orderStatus();
	static void delivery();
	static void stockLevel();

	/*
	 * Non Unified Random Number Generator
	 */
	static int NonURand(int A, int x, int y);
};

} /* namespace vt_dstm */

#endif /* TPCCOPS_H_ */
