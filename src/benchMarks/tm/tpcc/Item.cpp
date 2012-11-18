/*
 * Item.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "Item.h"
#include "TPCCOps.h"

namespace vt_dstm {

Item::Item() {
	// TODO Auto-generated constructor stub

}

Item::~Item() {
	// TODO Auto-generated destructor stub
}

int Item::getRandomItem() {
	return TPCC_Ops::NonURand(8191, 0, 100000)%100000;
}

int Item::getRandomQuantity() {
	return TPCC_Ops::NonURand(7, 0, 10)%10;
}

} /* namespace vt_dstm */
