/*
 * TpccItem.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccItem.h"
#include "TpccOps.h"
#include "../../../util/networking/NetworkManager.h"
#include <string>
#include <iostream>

#define TPCC_ITEM_ID_OFFSET 0

namespace vt_dstm {

TpccItem::TpccItem(int id) {
	I_ID = id ;
	int nodeId = NetworkManager::getNodeId();
	I_IM_ID = nodeId*I_ID + I_ID;
	I_NAME = "ItEm";
	I_PRICE = 2.0;
	I_DATA = "DaTa";
	std::stringstream IdStr;
	IdStr<<nodeId<<"-"<<I_ID + TPCC_ITEM_ID_OFFSET;
	hyId = IdStr.str();
}

TpccItem::~TpccItem() {}

std::string TpccItem::getItemId(int I_ID) {
	// Provide reference for local copy
	int nodeId = NetworkManager::getNodeId();
	std::stringstream IdStr;
	IdStr<<nodeId<<"-"<<I_ID + TPCC_ITEM_ID_OFFSET;
	return IdStr.str();
}

int TpccItem::getRandomItem() {
	return TpccOps::NonURand(8191, 0, 100000)%100000+1;
}

int TpccItem::getRandomQuantity() {
	return TpccOps::NonURand(7, 0, 10)%10;
}

void TpccItem::getClone(HyflowObject **obj) {
	TpccItem* ti = new TpccItem();
	ti->I_ID = I_ID;
	ti->I_IM_ID = I_IM_ID;
	ti->I_NAME = I_NAME;
	ti->I_PRICE = I_PRICE;
	ti->I_DATA = I_DATA;
	this->baseClone(ti);
	*obj = ti;
}

} /* namespace vt_dstm */
