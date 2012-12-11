/*
 * Vacation.h
 *
 *  Created on: Dec 5, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef VACATION_H_
#define VACATION_H_

#include <string>

namespace vt_dstm {

class Vacation {
public:
	Vacation();
	virtual ~Vacation();

	static void makeReservation(std::string& customerId, std::string resources[], int size);
	static void deleteCustomer(std::string & customerId);
	static void updateOffers(std::string resourceIds[], int prices[], int size);
};

} /* namespace vt_dstm */

#endif /* VACATION_H_ */
