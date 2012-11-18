/*
 * District.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "District.h"
#include "TPCCOps.h"

namespace vt_dstm {

District::District() {
	// TODO Auto-generated constructor stub

}

District::~District() {
	// TODO Auto-generated destructor stub
}

int District::getRandomDistrict() {
	return TPCC_Ops::NonURand(7,0,10)%10;
}

static std::string

} /* namespace vt_dstm */
