/*
 * dtl2Context.cpp
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#include "DTL2Context.h"

namespace vt_dstm {

DTL2Context::DTL2Context() {}

DTL2Context::~DTL2Context() {}

void DTL2Context::commit() {

}

unsigned long long DTL2Context::getTransactionId() {
	return 0;
}

} /* namespace vt_dstm */
