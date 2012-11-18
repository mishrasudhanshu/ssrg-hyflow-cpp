/*
 * Item.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ITEM_H_
#define ITEM_H_

#include "HyflowObject.h"

namespace vt_dstm {

class Item: public vt_dstm::HyflowObject {
public:
    int I_ID;	//		INTEGER,
    int I_IM_ID; //		INTEGER,
    std::string I_NAME;	 //		CHARACTER (24),
    float I_PRICE;		//	NUMERIC,
    std::string I_DATA; //	CHARACTER (50),

	Item();
	virtual ~Item();

    static std::string getItemId(int I_ID);
	static int getRandomItem();
	static int getRandomQuantity();
};

} /* namespace vt_dstm */

#endif /* ITEM_H_ */
