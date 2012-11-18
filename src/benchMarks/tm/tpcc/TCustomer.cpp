/*
 * TCustomer.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TCustomer.h"
#include "TPCCOps.h"

namespace vt_dstm {

TCustomer::TCustomer() {
	// TODO Auto-generated constructor stub

}

TCustomer::~TCustomer() {
	// TODO Auto-generated destructor stub
}

int TCustomer::getRandomCustomer() {
	return TPCC_Ops::NonURand(1023, 0, 3000)%3000;
}

} /* namespace vt_dstm */
