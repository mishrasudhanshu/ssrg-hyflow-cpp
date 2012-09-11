/*
 * busstop.h
 *
 *  Created on: Aug 21, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BUSSTOP_H_
#define BUSSTOP_H_

#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace vt_dstm {

class bus_stop {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & latitude;
        ar & longitude;
    }
protected:
    bus_stop(const int & _lat, const int & _long) :
        latitude(_lat), longitude(_long)
    {}
public:
    int latitude;
    int longitude;
    bus_stop(){}
    virtual ~bus_stop(){}
};

// Useful in case of some other type of compiler
BOOST_SERIALIZATION_ASSUME_ABSTRACT(bus_stop)

} /* namespace vt_dstm */

#endif /* BUSSTOP_H_ */
