/*
 * TpccItem.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef TPCCITEM_H_
#define TPCCITEM_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class TpccItem: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
		ar & I_ID;
		ar & I_IM_ID;
		ar & I_NAME;
		ar & I_PRICE;
		ar & I_DATA;
    }
public:
    int I_ID;	//		INTEGER,
    int I_IM_ID; //		INTEGER,
    std::string I_NAME;	 //		CHARACTER (24),
    float I_PRICE;		//	NUMERIC,
    std::string I_DATA; //	CHARACTER (50),

    TpccItem() {}
	TpccItem(int id);
	virtual ~TpccItem();

	void getClone(HyflowObject **obj);
	void print() {}

    static std::string getItemId(int I_ID);
	static int getRandomItem();
	static int getRandomQuantity();
};

} /* namespace vt_dstm */

#endif /* TPCCITEM_H_ */
