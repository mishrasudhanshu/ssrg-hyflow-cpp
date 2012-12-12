/*
 * TpccOrder.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccOrder.h"
#include "TpccBenchmark.h"

namespace vt_dstm {

TpccOrder::TpccOrder(int O_W_ID, int O_D_ID, int O_ID) {
	this->O_ID = O_ID;
	this->O_D_ID = O_D_ID;
	this->O_W_ID = O_W_ID;

	int oId = TpccBenchmark::getOrderBase(O_D_ID);
	std::stringstream idStr;
	idStr<<O_W_ID-1<<"-"<<oId+O_ID;
	hyId = idStr.str();
	hyVersion = 0;
}

TpccOrder::~TpccOrder() {}

std::string TpccOrder::getOrderId(int O_W_ID, int O_D_ID, int O_ID) {
	int oId = TpccBenchmark::getOrderBase(O_D_ID);
	std::stringstream idStr;
	idStr<<O_W_ID-1<<"-"<<oId+O_ID;
	return idStr.str();
}

void TpccOrder::getClone(HyflowObject **obj) {
	TpccOrder* to = new TpccOrder();
	to->O_ID = O_ID;
	to->O_D_ID = O_D_ID;
	to->O_W_ID = O_W_ID;
	to->O_C_ID = O_C_ID;
	to->O_ENTRY_D = O_ENTRY_D;
	to->O_CARRIER_ID = O_CARRIER_ID;
	to->O_OL_CNT = O_OL_CNT;
	to->O_ALL_LOCAL = O_ALL_LOCAL;
	this->baseClone(to);
	*obj = to;
}

} /* namespace vt_dstm */
