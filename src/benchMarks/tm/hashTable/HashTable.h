/*
 * HashMap.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

namespace vt_dstm {

class HashBucket: public vt_dstm::HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version){
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & bucketKeys;
    	ar & bucketValues;
    }

	std::vector<int > bucketKeys;
	std::vector<double> bucketValues;
	void putInternal(std::pair<int, double> entry);
	void removeInternal(int key);
	std::pair<int, double> getInternal(int key);
public:
	HashBucket();
	HashBucket(std::string id);
	virtual ~HashBucket();

	void print();
	void getClone(HyflowObject **obj);
	/*
	 * Adds the given value in the table
	 */
	static void put(std::pair<int, double> entry);
	/*
	 * Deletes the first occurrence of value in table
	 */
	static void remove(int value);
	/*
	 * Sums all the values in the list
	 */
	static void move(int key1, int key2);
	/*
	 * Finds the first occurrence of value in table
	 */
	static std::pair<int, double> get(int value);

	void test();
};

} /* namespace vt_dstm */

#endif /* HASHTABLE_H_ */
