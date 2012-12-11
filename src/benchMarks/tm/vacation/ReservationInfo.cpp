/*
 * ReservationInfo.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "ReservationInfo.h"
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../BenchmarkExecutor.h"
#include "VacationBenchmark.h"
#include "../../../core/helper/CheckPointProvider.h"

namespace vt_dstm {

ReservationInfo::ReservationInfo(std::string resources[], uint64_t prices[], int objectCount)
{
	std::stringstream idStr;
	int ownerNode = NetworkManager::getNodeId();
	idStr<<ownerNode<<"-"<<VacationBenchmark::getId();
	hyId = idStr.str();

	for(int objects=0 ; objects<objectCount ; objects++ ) {
		resourceIds.push_back(resources[objects]);
		resourcPrices.push_back(prices[objects]);
	}
	hyVersion = 0;
}

ReservationInfo::~ReservationInfo() {
	LOG_DEBUG("BANK : Delete Bank Account %s\n", hyId.c_str());
}

void ReservationInfo::print(){}

void ReservationInfo::getClone(HyflowObject **obj){
	ReservationInfo *ri = new ReservationInfo();
	ri->resourceIds = resourceIds;
	ri->resourcPrices = resourcPrices;
	this->baseClone(ri);
	*obj = ri;
}

void ReservationInfo::checkSanity(std::string* ids, int objectCount) {}

// Serialisation Test of object
void ReservationInfo::test() {}

} /* namespace vt_dstm */
