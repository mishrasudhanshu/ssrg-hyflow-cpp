/*
 * TpccCustomer.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccCustomer.h"
#include "TpccOps.h"

#define TPCC_CUSTOMER_ID_OFFSET 200000

namespace vt_dstm {

TpccCustomer::TpccCustomer(int warehouseId, int districtId, int customerId) {
	this->C_ID = customerId;
	this->C_D_ID = districtId;
	this->C_W_ID = warehouseId;
	std::stringstream baseStr;
	baseStr<<warehouseId<<"-"<<districtId<<"-"<<customerId;
	std::string baseName = baseStr.str();
	this->C_FIRST = baseName.append("-CUST");
	this->C_MIDDLE = baseName.append("-MID");
	this->C_LAST = baseName.append("-LAST");
	this->C_STREET_1 = baseName.append("-ST1");
	this->C_STREET_2 = baseName.append("-ST2");
	this->C_CITY = baseName.append("-CTY");
	this->C_STATE = baseName.append("-ST");
	this->C_ZIP = baseName.append("-ZIP");
	this->C_PHONE = baseName.append("-PH");
	this->C_SINCE = baseName.append("-SN");
	this->C_CREDIT = "CR";
	this->C_CREDIT_LIM = 10000;
	this->C_DISCOUNT = 1;
	this->C_BALANCE = 0;
	this->C_YTD_PAYMENT = 0;
	this->C_CNT_PAYMENT = 0;
	this->C_CNT_DELIVERY = 0;
	this->C_DATA_1 = "C_DATA_1";
	this->C_DATA_2 = "C_DATA_2";
	std::stringstream idStr;
	idStr<<(warehouseId-1)<<"-"<< (districtId-1)*3000 + customerId + TPCC_CUSTOMER_ID_OFFSET;
	hyId = idStr.str();
	hyVersion = 0;
}

TpccCustomer::~TpccCustomer() {}

std::string TpccCustomer::getCustomerId(int warehouseId, int districtId, int customerId) {
	std::stringstream idStr;
	idStr<<(warehouseId-1)<<"-"<<(districtId-1)*3000 + customerId + TPCC_CUSTOMER_ID_OFFSET;
	return idStr.str();
}

int TpccCustomer::getRandomCustomer() {
	return TpccOps::NonURand(1023, 0, 3000)%3000+1;
}

void TpccCustomer::getClone(HyflowObject **tCustomer) {
	TpccCustomer *tc = new TpccCustomer();
	tc->C_ID = C_ID;
	tc->C_D_ID = C_D_ID;
	tc->C_W_ID = C_W_ID;
	tc->C_FIRST = C_FIRST;
	tc->C_MIDDLE = C_MIDDLE;
	tc->C_LAST = C_LAST;
	tc->C_STREET_1 = C_STREET_1;
	tc->C_STREET_2 = C_STREET_2;
	tc->C_CITY = C_CITY;
	tc->C_STATE = C_STATE;
	tc->C_ZIP = C_ZIP;
	tc->C_PHONE = C_PHONE;
	tc->C_SINCE = C_SINCE;
	tc->C_CREDIT = C_CREDIT;
	tc->C_CREDIT_LIM = C_CREDIT_LIM;
	tc->C_DISCOUNT = C_DISCOUNT;
	tc->C_BALANCE = C_BALANCE;
	tc->C_YTD_PAYMENT = C_YTD_PAYMENT;
	tc->C_CNT_PAYMENT = C_CNT_PAYMENT;
	tc->C_CNT_DELIVERY = C_CNT_DELIVERY;
	tc->C_DATA_1 = C_DATA_1;
	tc->C_DATA_2 = C_DATA_2;
	this->baseClone(tc);
	*tCustomer = tc;
}
} /* namespace vt_dstm */
