/*
 * ReservationInfo.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef RESERVATIONINFO_H_
#define RESERVATIONINFO_H_

#include <stdint.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

#define AMOUNT 100000

namespace vt_dstm {

class ReservationInfo: public HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & resourceIds;
    	ar & resourcPrices;
    }

    std::vector<std::string> resourceIds;
    std::vector<uint64_t> resourcPrices;

public:
	ReservationInfo() {};
	ReservationInfo(std::string resources[], uint64_t prices[], int objectCount);
	virtual ~ReservationInfo();

	void print();
	void getClone(HyflowObject **obj);
	static void checkSanity(std::string* ids, int objectCount);
	void test();
};

} /* namespace vt_dstm */

#endif /* RESERVATIONINFO_H_ */
