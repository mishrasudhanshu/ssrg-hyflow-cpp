/*
 * TPCCWareHouse.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccWareHouse.h"
#include "TpccOps.h"
#include "../../../util/networking/NetworkManager.h"

namespace vt_dstm {

TpccWareHouse::TpccWareHouse(int W_ID) {
	this->W_ID = W_ID;
	this->W_NAME = W_ID+"-W";
	this->W_STREET_1 = W_ID+"_ST1";
	this->W_STREET_2 = W_ID+"_ST2";
	this->W_CITY = W_ID+"_CTY";
	this->W_STATE = W_ID+"_ST";
	this->W_ZIP = W_ID+"0000";
	this->W_TAX = 1;
	this->W_YTD = 0;
	std::stringstream idStr;
	idStr<<W_ID-1<<"-0";
	hyId = idStr.str();
	hyVersion = 0;
}

TpccWareHouse::~TpccWareHouse() {}

std::string TpccWareHouse::getWareHouseId(int wareHouse) {
	std::stringstream idStr;
	idStr<<wareHouse-1<<"-0";
	return idStr.str();
}


void TpccWareHouse::getClone(HyflowObject **obj) {
	TpccWareHouse* tw = new TpccWareHouse();
	tw->W_ID	=	W_ID;
	tw->W_NAME	=	W_NAME;
	tw->W_STREET_1	=	W_STREET_1;
	tw->W_STREET_2	=	W_STREET_2;
	tw->W_CITY	=	W_CITY;
	tw->W_STATE	=	W_STATE;
	tw->W_ZIP	=	W_ZIP;
	tw->W_TAX	=	W_TAX;
	tw->W_YTD	=	W_YTD;
	this->baseClone(tw);
	*obj = tw;
}

int TpccWareHouse::getRandomWareHouse() {
	int warehouses = NetworkManager::getNodeCount();
	return TpccOps::NonURand(7, 0, warehouses)%warehouses+1;
}

} /* namespace vt_dstm */
