/*
 * MSCtest.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef MSCTEST_H_
#define MSCTEST_H_

namespace vt_dstm{

class MSCtest {
public:
	static volatile bool hyShutdown;
	static void test();
};

} /* namespace vt_dstm */


#endif /* MSCTEST_H_ */
