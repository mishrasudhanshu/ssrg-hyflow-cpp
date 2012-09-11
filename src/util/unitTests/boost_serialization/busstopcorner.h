/*
 * busstopcorner.h
 *
 *  Created on: Aug 21, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BUSSTOPCORNER_H_
#define BUSSTOPCORNER_H_

#include "bus_stop.h"

namespace vt_dstm {

class bus_stop_corner : public bus_stop
{
    friend class boost::serialization::access;
    std::string street1;
    std::string street2;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        // save/load base class information
        ar & boost::serialization::base_object<bus_stop>(*this);
        ar & street1 & street2;
    }

public:
    bus_stop_corner(){}
    bus_stop_corner(const int & _lat, const int & _long,
        const std::string & _s1, const std::string & _s2
    );

    void print();

    /*
     * Very basic test to verify the boost serialization sanity
     */
    void test();
};
} /* namespace vt_dstm */

#endif /* BUSSTOPCORNER_H_ */
