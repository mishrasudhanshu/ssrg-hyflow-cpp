/*
 * District.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DISTRICT_H_
#define DISTRICT_H_

#include "HyflowObject.h"

namespace vt_dstm {

class District: public vt_dstm::HyflowObject {
public:
    int D_ID;		//INTEGER,
    int D_W_ID;		//INTEGER,
    int D_NAME;		//CHARACTER (10),
    std::string D_STREET_1;		//CHARACTER (20),
    std::string D_STREET_2;		//CHARACTER (20),
    std::string D_CITY;		//CHARACTER (20),
    std::string D_STATE;	//CHARACTER (2),
    std::string D_ZIP;		//CHARACTER (9),
    float D_TAX;		//NUMERIC,
    float D_YTD;		//NUMERIC,
    int D_NEXT_O_ID;		//INTEGER,
    int D_LAST_DELV_O_ID;	//Added By me
	District();
	virtual ~District();

	static int getRandomDistrict();
	static std::string getDistrictId(int D_W_ID, int D_ID);
};

} /* namespace vt_dstm */

#endif /* DISTRICT_H_ */
