/*
 * TpccDistrict.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCDISTRICT_H_
#define TPCCDISTRICT_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccDistrict: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & D_ID;
		ar & D_W_ID;
		ar & D_NAME;
		ar & D_STREET_1;
		ar & D_STREET_2;
		ar & D_CITY;
		ar & D_STATE;
		ar & D_ZIP;
		ar & D_TAX;
		ar & D_YTD;
		ar & D_NEXT_O_ID;
		ar & D_LAST_DELV_O_ID;
		ar & D_ZIP;
		ar & D_TAX;
		ar & D_YTD;
		ar & D_NEXT_O_ID;
		ar & D_LAST_DELV_O_ID;
    }
public:
    int D_ID;		//INTEGER,
    int D_W_ID;		//INTEGER,
    std::string D_NAME;		//CHARACTER (10),
    std::string D_STREET_1;		//CHARACTER (20),
    std::string D_STREET_2;		//CHARACTER (20),
    std::string D_CITY;		//CHARACTER (20),
    std::string D_STATE;	//CHARACTER (2),
    std::string D_ZIP;		//CHARACTER (9),
    float D_TAX;		//NUMERIC,
    float D_YTD;		//NUMERIC,
    int D_NEXT_O_ID;		//INTEGER,
    int D_LAST_DELV_O_ID;	//Added By me to keep track of processed records

	TpccDistrict() {}
	TpccDistrict(int D_W_ID, int D_ID);
	virtual ~TpccDistrict();

	void print() {}
	void getClone(HyflowObject **obj);
	static int getRandomDistrict();
	static std::string getDistrictId(int D_W_ID, int D_ID);
};

} /* namespace vt_dstm */

#endif /* TPCCDISTRICT_H_ */
