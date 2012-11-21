/*
 * TpccNewOrder.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccNewOrder.h"
#include "TpccBenchmark.h"

namespace vt_dstm {

TpccNewOrder::TpccNewOrder(int NO_W_ID, int NO_D_ID, int NO_O_ID) {
	this->NO_O_ID = NO_O_ID;
	this->NO_D_ID = NO_D_ID;
	this->NO_W_ID = NO_W_ID;
	int noId = TpccBenchmark::getOrderBase(NO_D_ID);
	std::stringstream idStr;
	idStr<<NO_W_ID-1<<"-"<<noId+NO_O_ID;
	hyId = idStr.str();
}

TpccNewOrder::~TpccNewOrder() {}

std::string TpccNewOrder::getNewOrderId(int NO_W_ID, int NO_D_ID, int NO_O_ID) {
	int noId = TpccBenchmark::getOrderBase(NO_D_ID);
	std::stringstream idStr;
	idStr<<NO_W_ID-1<<"-"<<noId+NO_O_ID;
	return idStr.str();
}

void TpccNewOrder::getClone(HyflowObject **obj) {
	TpccNewOrder* tno = new TpccNewOrder();
  	tno->NO_D_ID = NO_O_ID;
  	tno->NO_D_ID = NO_D_ID;
  	tno->NO_W_ID = NO_W_ID;
	this->baseClone(tno);
	*obj = tno;
}

} /* namespace vt_dstm */
