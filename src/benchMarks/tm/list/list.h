/*
 * list.h
 *
 *  Created on: Aug 22, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LIST_H_
#define LIST_H_

#include "../../../core/HyflowObject.h"

namespace vt_dstm {

class list:
	public HyflowObject
{
	friend class boost::serialization::access;
	std::string street1;
	std::string street2;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
public:
	list(){}
	list(const std::string & Id, const int & v, const std::string & s1, const std::string &s2);

	virtual ~list(){};
	void print();
};

} /* namespace vt_dstm */

#endif /* LIST_H_ */
