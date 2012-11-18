/*
 * Stock.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef STOCK_H_
#define STOCK_H_

#include "HyflowObject.h"

namespace vt_dstm {

class Stock: public vt_dstm::HyflowObject {
public:
    int S_I_ID;			//INTEGER,
    int S_W_ID;			//INTEGER,
    float S_QUANTITY;	//NUMERIC,
    std::string S_DIST_01;		//CHARACTER (24),
    std::string S_DIST_02;		//CHARACTER (24),
    std::string S_DIST_03;		//CHARACTER (24),
    std::string S_DIST_04;		//CHARACTER (24),
    std::string S_DIST_05;		//CHARACTER (24),
    std::string S_DIST_06;		//CHARACTER (24),
    std::string S_DIST_07;		//CHARACTER (24),
    std::string S_DIST_08;		//CHARACTER (24),
    std::string S_DIST_09;		//CHARACTER (24),
    std::string S_DIST_10;		//CHARACTER (24),
    std::string S_YTD;		//NUMERIC,
    std::string S_CNT_ORDER;		//NUMERIC,
    std::string S_CNT_REMOTE;	//NUMERIC,
    std::string S_DATA;		//CHARACTER (50),

    static std::string getStockId(int S_W_ID, int S_I_ID);
    static float getRandomThreshold();

	Stock();
	virtual ~Stock();
};

} /* namespace vt_dstm */

#endif /* STOCK_H_ */
