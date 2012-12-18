/*
 * Vacation.cpp
 *
 *  Created on: Dec 5, 2012
 *      Author: mishras[at]vt.edu
 */

#include "Vacation.h"
#include "../../../core/helper/Atomic.h"

#include "Resource.h"
#include "ReservationInfo.h"
#include "Customer.h"
#include "../../../util/logging/Logger.h"
#include "../../BenchmarkExecutor.h"


namespace vt_dstm {

Vacation::Vacation() {}

Vacation::~Vacation() {}

void Vacation::makeReservation(std::string& customerId, std::string resources[], int size) {
	uint64_t minPrices[3] = {0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff};
	std::string objects[3];
	HYFLOW_ATOMIC_START {
		for(int queries=0; queries<size ; queries++ ) {
			LOG_DEBUG("Vacation :makeReservation : fetching resource %s\n", resources[queries].c_str());
			HYFLOW_FETCH(resources[queries], true);
			Resource* res = (Resource*)HYFLOW_ON_READ(resources[queries]);
			uint64_t price = res->getPrice();
			ResourceType type = res->getResouceType();
			if ( price < minPrices[type]) {
				objects[type] = res->getId();
			}
		}

		//Create the reversion info
		ReservationInfo* resInfo = new ReservationInfo(objects, minPrices, 3);
		LOG_DEBUG("Vacation :makeReservation created reservation info %s\n", resInfo->getId().c_str());
		HYFLOW_PUBLISH_OBJECT(resInfo);

		// Fetch customer
		bool isAvailable = HYFLOW_NFETCH(customerId, false, false);
		if (isAvailable) {
			LOG_DEBUG("Vacation :makeReservation updating customer %s\n", customerId.c_str());
			Customer* customer = (Customer*) HYFLOW_ON_WRITE(customerId);
			customer->addReseverationInfo(resInfo);
		}else {
			LOG_DEBUG("Vacation :makeReservation Customer Already Deleted\n");
		}
	} HYFLOW_ATOMIC_END;
}

void Vacation::deleteCustomer(std::string & customerId) {
	HYFLOW_ATOMIC_START {
		// Fetch customer
		bool isAvailable = HYFLOW_NFETCH(customerId, false, false);
		if (isAvailable) {
			LOG_DEBUG("Vacation :Deleting Customer %s\n", customerId.c_str());
			Customer* customer = (Customer*) HYFLOW_ON_WRITE(customerId);
			HYFLOW_DELETE_OBJECT(customer);
		}else {
			LOG_DEBUG("Vacation :Customer %s Already Deleted\n", customerId.c_str());
		}
	}HYFLOW_ATOMIC_END;
}

void Vacation::updateOffers(std::string resources[], int prices[], int size) {
	HYFLOW_ATOMIC_START {
		for(int queries=0; queries<size ; queries++ ) {
			LOG_DEBUG("Vacation :Updating offer for %s by %d\n", resources[queries].c_str(), prices[queries]);
			HYFLOW_FETCH(resources[queries], false);
			Resource* res = (Resource*)HYFLOW_ON_READ(resources[queries]);
			res->setPrice(prices[queries]);
		}
	} HYFLOW_ATOMIC_END;
}

} /* namespace vt_dstm */
