/*
 * TpccOrderLine.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccOrderLine.h"
#include "TpccBenchmark.h"

namespace vt_dstm {

TpccOrderLine::	TpccOrderLine(int w_id, int d_id, int o_id, int ol_num) {
	this->OL_W_ID = w_id;
	this->OL_D_ID = d_id;
	this->OL_O_ID = o_id;
	this->OL_NUMBER = ol_num;

	int olId = TpccBenchmark::getOrderLineBase(OL_D_ID);
	std::stringstream idStr;
	idStr<<OL_W_ID-1<<"-"<<olId+(OL_O_ID-1)*20+OL_NUMBER;
	hyId = idStr.str();
	hyVersion = 0;
}

std::string TpccOrderLine::getOrderLineId(int OL_W_ID, int OL_D_ID, int OL_O_ID, int OL_NUMBER) {
	int olId = TpccBenchmark::getOrderLineBase(OL_D_ID);
	std::stringstream idStr;
	idStr<<OL_W_ID-1<<"-"<<olId+(OL_O_ID-1)*20+OL_NUMBER;
	return idStr.str();
}

TpccOrderLine::~TpccOrderLine() {}

void TpccOrderLine::getClone(HyflowObject **obj) {
	TpccOrderLine* tol = new TpccOrderLine();
	tol->OL_O_ID = OL_O_ID;
	tol->OL_D_ID = OL_D_ID;
	tol->OL_W_ID = OL_W_ID;
	tol->OL_NUMBER = OL_NUMBER;
	tol->OL_I_ID = OL_I_ID;
	tol->OL_SUPPLY_W_ID = OL_SUPPLY_W_ID;
	tol->OL_DELIVERY_D = OL_DELIVERY_D;
	tol->OL_QUANTITY = OL_QUANTITY;
	tol->OL_AMOUNT = OL_AMOUNT;
	tol->OL_DIST_INFO = OL_DIST_INFO;
	this->baseClone(tol);
	*obj = tol;
}

} /* namespace vt_dstm */
