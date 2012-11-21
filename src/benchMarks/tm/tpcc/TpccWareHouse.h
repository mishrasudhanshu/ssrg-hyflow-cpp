/*
 * TpccWareHouse.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCWAREHOUSE_H_
#define TPCCWAREHOUSE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccWareHouse: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & W_ID;
		ar & W_NAME;
		ar & W_STREET_1;
		ar & W_STREET_2;
		ar & W_CITY;
		ar & W_STATE;
		ar & W_ZIP;
		ar & W_TAX;
		ar & W_YTD;
    }
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

	TpccWareHouse() {}
	TpccWareHouse(int W_ID);
	virtual ~TpccWareHouse();

	void print() {}
	void getClone(HyflowObject **obj);
	static std::string getWareHouseId(int wareHouse);
	static int getRandomWareHouse();
};

} /* namespace vt_dstm */

#endif /* TPCCWAREHOUSE_H_ */
