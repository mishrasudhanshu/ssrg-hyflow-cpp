/*
 * TpccBenchmark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccBenchmark.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../util/concurrent/ThreadMeta.h"

#include "TpccItem.h"
#include "TpccOps.h"

#define TPCC_ORDER_ID_OFFSET 10000000
#define TPCC_NEW_ORDER_ID_OFFSET 20000000
#define TPCC_ORDER_LINE_ID_OFFSET 30000000
#define TPCC_ORDER_HISTORY_ID_OFFSET 40000000
#define TPCC_ID_STEP 1000000

namespace vt_dstm {

boost::thread_specific_ptr<HyInteger> TpccBenchmark::historyCreated;

TpccBenchmark::TpccBenchmark() {}

TpccBenchmark::~TpccBenchmark() {
	delete ids;
}

int TpccBenchmark::getOperandsCount() {
	return 2;
}

int TpccBenchmark::getOrderBase(int district) {
	return (district-1)*TPCC_ID_STEP + TPCC_ORDER_ID_OFFSET;
}

int TpccBenchmark::getNewOrderBase(int district) {
	return (district-1)*TPCC_ID_STEP + TPCC_NEW_ORDER_ID_OFFSET;
}

int TpccBenchmark::getOrderLineBase(int district) {
	return (district-1)*TPCC_ID_STEP + TPCC_ORDER_LINE_ID_OFFSET;
}

int TpccBenchmark::getNextHistory() {
	HyInteger* objectCount = historyCreated.get();
	if (!objectCount) {
		historyCreated.reset(new HyInteger(0));
		historyCreated.get()->setValue(ThreadMeta::getThreadId()*TPCC_ID_STEP + TPCC_ORDER_HISTORY_ID_OFFSET);
	}
	historyCreated->increaseValue();
	return historyCreated.get()->getValue();
}

void TpccBenchmark::readOperation(std::string ids[], int size) {
	TpccOperation();
}

void TpccBenchmark::writeOperation(std::string ids[], int size) {
	TpccOperation();
}

void TpccBenchmark::TpccOperation() {
	int opt = abs(Logger::getCurrentMicroSec())%100;
	if (opt < 4) {
			LOG_DEBUG("Run Transaction: Order Status\n");
			TpccOps::orderStatus();
	} else if (opt < 8) {
			LOG_DEBUG("Run Transaction: Delivery\n");
			TpccOps::delivery();
	} else if (opt < 12) {
			LOG_DEBUG("Run Transaction: Stock Level\n");
			TpccOps::stockLevel();
	} else if (opt < 55) {
			LOG_DEBUG("Run Transaction: Payment\n");
			TpccOps::payment();
	} else {
			LOG_DEBUG("Run Transaction: New Order\n");
			TpccOps::newOrder();
	}
}

void TpccBenchmark::checkSanity() {}

std::string* TpccBenchmark::createLocalObjects(int objCount) {
	/*
	 * For One ware house - 1 ware house per node
	 * 100K item
	 * 100k stock 1 stock per item
	 * 10 district per warehouse
	 * 3k customers per district
	 * Total 300k+ objects per warehouse
	 * After that any new type of item Id
	 * For orders 100K objects
	 * For new Orders 100K objects
	 * For histories 100K objects
	 * For OrderLines values upward
	 */
	objectCount = objCount;
	ids = new std::string [objCount];
	int nodeId = NetworkManager::getNodeId();

	// One warehouse per Node start value from 1
	int warehouseId = nodeId+1;
	TpccWareHouse warehouse(warehouseId);
	DirectoryManager::registerObjectWait(&warehouse, 0);

	// Create all the items and there stocks
	LOG_DEBUG("TPCCB :Creating Items and stock\n");
	for(int itemCount=0 ; itemCount<100000 ; itemCount++ ) {
		TpccItem item(itemCount+1);
		DirectoryManager::registerObjectWait(&item, 0);
		TpccStock stock(warehouseId, itemCount+1);
		DirectoryManager::registerObjectWait(&stock, 0);
	}

	for(int districtCount=0 ; districtCount < 10 ; districtCount++ ) {
		LOG_DEBUG("TPCCB :Creating District %d\n", districtCount+1);
		TpccDistrict district(warehouseId, districtCount+1);
		DirectoryManager::registerObjectWait(&district, 0);
		for ( int customerCount=0 ; customerCount<3000 ; customerCount++ ) {
			TpccCustomer customer(warehouseId, districtCount+1, customerCount+1);
			DirectoryManager::registerObjectWait(&customer, 0);
		}
	}

	// TODO : Don't Provide Random Ids, we don't need
	for (int i = 0; i < objCount; i++) {
		std::ostringstream idStream;
		idStream << 0 << "-" << 0;
		ids[i] = idStream.str();
	}

	return ids;
}

} /* namespace vt_dstm */
