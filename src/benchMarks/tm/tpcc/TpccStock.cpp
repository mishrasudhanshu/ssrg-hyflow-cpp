/*
 * TpccStock.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccStock.h"

#define TPCC_STOCK_ID_OFFSET 100000

namespace vt_dstm {

TpccStock::TpccStock() {}

TpccStock::TpccStock(int S_W_ID, int S_I_ID) {
	// assert(S_W_ID == nodeId)
	this->S_I_ID = S_I_ID;
	this->S_W_ID = S_W_ID;
	this->S_QUANTITY = 1000;
	this->S_DIST_01 = S_W_ID + "-1";
	this->S_DIST_02 = S_W_ID + "-2";
	this->S_DIST_03 = S_W_ID + "-3";
	this->S_DIST_04 = S_W_ID + "-4";
	this->S_DIST_05 = S_W_ID + "-5";
	this->S_DIST_06 = S_W_ID + "-6";
	this->S_DIST_07 = S_W_ID + "-7";
	this->S_DIST_08 = S_W_ID + "-8";
	this->S_DIST_09 = S_W_ID + "-9";
	this->S_DIST_10 = S_W_ID + "-10";
	this->S_YTD = 0;
	this->S_CNT_ORDER = 0;
	this->S_CNT_REMOTE = 0;
	this->S_DATA = "DaTa";
	std::stringstream IdStr;
	IdStr<<S_W_ID-1<<"-"<<S_I_ID + TPCC_STOCK_ID_OFFSET;
	hyId = IdStr.str();
}

TpccStock::~TpccStock() {
	// TODO Auto-generated destructor stub
}

std::string TpccStock::getStockId(int S_W_ID, int S_I_ID) {
	std::stringstream IdStr;
	IdStr<<S_W_ID-1<<"-"<<S_I_ID + TPCC_STOCK_ID_OFFSET;
	return IdStr.str();
}

void TpccStock::getClone(HyflowObject **obj) {
	TpccStock* ts = new TpccStock();
	ts->S_I_ID = S_I_ID;
	ts->S_W_ID = S_W_ID;
	ts->S_QUANTITY = S_QUANTITY;
	ts->S_DIST_01 = S_DIST_01;
	ts->S_DIST_02 = S_DIST_02;
	ts->S_DIST_03 = S_DIST_03;
	ts->S_DIST_04 = S_DIST_04;
	ts->S_DIST_05 = S_DIST_05;
	ts->S_DIST_06 = S_DIST_06;
	ts->S_DIST_07 = S_DIST_07;
	ts->S_DIST_08 = S_DIST_08;
	ts->S_DIST_09 = S_DIST_09;
	ts->S_DIST_10 = S_DIST_10;
	ts->S_YTD = S_YTD;
	ts->S_CNT_ORDER = S_CNT_ORDER;
	ts->S_CNT_REMOTE = S_CNT_REMOTE;
	ts->S_DATA = S_DATA;
	this->baseClone(ts);
	*obj = ts;
}

} /* namespace vt_dstm */
