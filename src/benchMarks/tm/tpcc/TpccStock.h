/*
 * TpccStock.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCSTOCK_H_
#define TPCCSTOCK_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccStock: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & S_I_ID;
		ar & S_W_ID;
		ar & S_QUANTITY;
		ar & S_DIST_01;
		ar & S_DIST_02;
		ar & S_DIST_03;
		ar & S_DIST_04;
		ar & S_DIST_05;
		ar & S_DIST_06;
		ar & S_DIST_07;
		ar & S_DIST_08;
		ar & S_DIST_09;
		ar & S_DIST_10;
		ar & S_YTD;
		ar & S_CNT_ORDER;
		ar & S_CNT_REMOTE;
		ar & S_DATA;
    }
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
    float S_YTD;		//NUMERIC,
    float S_CNT_ORDER;		//NUMERIC,
    float S_CNT_REMOTE;	//NUMERIC,
    std::string S_DATA;		//CHARACTER (50),

    TpccStock();
	TpccStock(int S_W_ID, int S_I_ID);
	virtual ~TpccStock();

	static std::string getStockId(int S_W_ID, int S_I_ID);
    static float getRandomThreshold() {return 20; }

    void getClone(HyflowObject** obj);
    void print() {}
};

} /* namespace vt_dstm */

#endif /* TPCCSTOCK_H_ */
