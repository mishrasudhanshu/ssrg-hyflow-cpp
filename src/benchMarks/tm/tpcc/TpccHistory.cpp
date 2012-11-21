/*
 * TpccHistory.cpp
 *
 *  Created on: Nov 18, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccHistory.h"
#include "TpccBenchmark.h"

#include "TpccOps.h"
#include "../../../util/networking/NetworkManager.h"

namespace vt_dstm {

TpccHistory::TpccHistory(int H_W_ID, int H_D_ID) {
	this->H_D_ID = H_D_ID;
	this->H_W_ID = H_W_ID;
	this->H_DATE = "H_DATE";
	this->H_AMOUNT = 1;
	this->H_DATA = "H_DATA";
	int hId = TpccBenchmark::getNextHistory();
	std::stringstream idStr;
	idStr<<NetworkManager::getNodeId()<<"-"<<hId;
	hyId = idStr.str();
}

TpccHistory::~TpccHistory() {}

float TpccHistory::getRandomAmount() {
	return TpccOps::NonURand(7, 0, 10)%10;
}

void TpccHistory::getClone(HyflowObject **obj) {
	TpccHistory* ht = new TpccHistory();
	ht->H_C_ID = H_C_ID;
	ht->H_C_D_ID = H_C_D_ID;
	ht->H_C_W_ID = H_C_W_ID;
	ht->H_D_ID = H_D_ID;
	ht->H_W_ID = H_W_ID;
	ht->H_DATE = H_DATE;
	ht->H_AMOUNT = H_AMOUNT;
	ht->H_DATA = H_DATA;
	this->baseClone(ht);
	*obj = ht;
}

} /* namespace vt_dstm */
