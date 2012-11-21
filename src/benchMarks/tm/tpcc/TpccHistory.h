/*
 * TpccHistory.h
 *
 *  Created on: Nov 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCHISTORY_H_
#define TPCCHISTORY_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccHistory: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & H_C_ID;
		ar & H_C_D_ID;
		ar & H_C_W_ID;
		ar & H_D_ID;
		ar & H_W_ID;
		ar & H_DATE;
		ar & H_AMOUNT;
		ar & H_DATA;
    }
public:
    int H_C_ID;		//INTEGER,
    int H_C_D_ID;		//INTEGER,
    int H_C_W_ID;		//INTEGER,
    int H_D_ID;		//INTEGER,
    int H_W_ID;		//INTEGER,
    std::string H_DATE;		//DATE,
    int H_AMOUNT;	//NUMERIC,
    std::string H_DATA;		//CHARACTER (24),
	TpccHistory() {};
	TpccHistory(int H_W_ID, int H_D_ID);

	virtual ~TpccHistory();

	void print() {}
	void getClone(HyflowObject** obj);
	static float getRandomAmount();
};

} /* namespace vt_dstm */

#endif /* TPCCHISTORY_H_ */
