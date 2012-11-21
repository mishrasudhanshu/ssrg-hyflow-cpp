/*
 * District.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccDistrict.h"
#include "TpccOps.h"

#define TPCC_DISTRICT_ID_OFFSET 230000

namespace vt_dstm {

TpccDistrict::TpccDistrict(int D_W_ID, int D_ID) {
	this->D_ID = D_ID;
	this->D_W_ID = D_W_ID;
	std::stringstream baseStr;
	baseStr<<D_W_ID<<"-"<<D_ID;
	std::string base=baseStr.str();
	this->D_NAME = base;
	this->D_STREET_1 = base.append("-ST1");
	this->D_STREET_2 = base.append("-ST2");
	this->D_CITY = "D_CITY";
	this->D_STATE = "D_STATE";
	this->D_ZIP = "D_ZIP";
	this->D_TAX = 1;
	this->D_YTD = 0;
	this->D_NEXT_O_ID = 1;
	this->D_LAST_DELV_O_ID = 1;
	std::stringstream idStr;
	idStr<<D_W_ID-1<<"-"<<D_ID + TPCC_DISTRICT_ID_OFFSET;
	hyId = idStr.str();
}

TpccDistrict::~TpccDistrict() {}


std::string TpccDistrict::getDistrictId(int D_W_ID, int D_ID) {
	std::stringstream idStr;
	idStr<<D_W_ID-1<<"-"<<D_ID + TPCC_DISTRICT_ID_OFFSET;
	return idStr.str();
}
int TpccDistrict::getRandomDistrict() {
	return TpccOps::NonURand(7,0,10)%10+1;
}

void TpccDistrict::getClone(HyflowObject **obj) {
	TpccDistrict* td = new TpccDistrict();
	td->D_ID = D_ID;
	td->D_W_ID = D_W_ID;
	td->D_NAME = D_NAME;
	td->D_STREET_1 = D_STREET_1;
	td->D_STREET_2 = D_STREET_2;
	td->D_CITY = D_CITY;
	td->D_STATE = D_STATE;
	td->D_ZIP = D_ZIP;
	td->D_TAX = D_TAX;
	td->D_YTD = D_YTD;
	td->D_NEXT_O_ID = D_NEXT_O_ID;
	td->D_LAST_DELV_O_ID = D_LAST_DELV_O_ID;
	this->baseClone(td);
	*obj = td;
}

} /* namespace vt_dstm */
