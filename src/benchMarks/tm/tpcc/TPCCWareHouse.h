/*
 * TPCCWareHouse.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCWAREHOUSE_H_
#define TPCCWAREHOUSE_H_

#include "HyflowObject.h"

namespace vt_dstm {

class TPCCWareHouse: public vt_dstm::HyflowObject {
public:
    int W_ID;				//	INTEGER,
    std::string W_NAME;		//	CHARACTER (10),
    std::string W_STREET_1;	//	CHARACTER (20),
    std::string	W_STREET_2;	//	CHARACTER (20),
    std::string W_CITY;		//	CHARACTER (20),
    std::string W_STATE;	//	CHARACTER (2),
    std::string W_ZIP;		//	CHARACTER (9),
    float		W_TAX;		//	NUMERIC,
    float 		W_YTD;		//	NUMERIC,

	TPCCWareHouse();
	virtual ~TPCCWareHouse();

	static std::string getWareHouseId(int wareHouse);
	static int getRandomWareHouse();
};

} /* namespace vt_dstm */

#endif /* TPCCWAREHOUSE_H_ */
