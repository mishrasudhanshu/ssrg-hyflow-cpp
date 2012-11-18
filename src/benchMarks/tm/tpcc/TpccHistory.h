/*
 * TpccHistory.h
 *
 *  Created on: Nov 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCHISTORY_H_
#define TPCCHISTORY_H_

#include "HyflowObject.h"

namespace vt_dstm {

class TpccHistory: public vt_dstm::HyflowObject {
public:
    int H_C_ID;		//INTEGER,
    int H_C_D_ID;		//INTEGER,
    int H_C_W_ID;		//INTEGER,
    int H_D_ID;		//INTEGER,
    int H_W_ID;		//INTEGER,
    std::string H_DATE;		//DATE,
    int H_AMOUNT;	//NUMERIC,
    std::string H_DATA;		//CHARACTER (24),
	TpccHistory();
	virtual ~TpccHistory();

	void print() {}
	void getClone(HyflowObject** obj);
	static float getRandomAmount();
    static std::string getHistory(std::string H_DATE, std::string H_C_ID)
};

} /* namespace vt_dstm */

#endif /* TPCCHISTORY_H_ */
