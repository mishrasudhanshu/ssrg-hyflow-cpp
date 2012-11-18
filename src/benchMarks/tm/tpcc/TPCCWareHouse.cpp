/*
 * TPCCWareHouse.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TPCCWareHouse.h"
#include "TPCCOps.h"

#define TPCC_WARE_HOUSES 2

namespace vt_dstm {

TPCCWareHouse::TPCCWareHouse() {
	// TODO Auto-generated constructor stub

}

TPCCWareHouse::~TPCCWareHouse() {
	// TODO Auto-generated destructor stub
}

static int TPCCWareHouse::getRandomWareHouse() {
	return TPCC_Ops::NonURand(7, 0, TPCC_WARE_HOUSES)%TPCC_WARE_HOUSES;
}

} /* namespace vt_dstm */
