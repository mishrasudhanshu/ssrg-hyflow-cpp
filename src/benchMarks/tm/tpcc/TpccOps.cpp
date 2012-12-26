/*
 * TpccOps.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TpccOps.h"
#include <set>
#include "../../../core/helper/Atomic.h"
#include "../../../util/logging/Logger.h"
#include "../../BenchmarkExecutor.h"

#include "TpccOrder.h"
#include "TpccDistrict.h"
#include "TpccItem.h"
#include "TpccNewOrder.h"
#include "TpccOrderLine.h"
#include "TpccStock.h"
#include "TpccCustomer.h"
#include "TpccWareHouse.h"
#include "TpccHistory.h"

#define TPCC_CONSTANT 10

namespace vt_dstm {

TpccOps::TpccOps() {}

TpccOps::~TpccOps() {}

int TpccOps::NonURand(int A, int x, int y) {
	return ((abs(Logger::getCurrentMicroSec())%A)|(x+abs(Logger::getCurrentMicroSec())%(y-x)+TPCC_CONSTANT)%(y-x+1))+x;
}


double TpccOps::newOrder() {
	//Create WorkLoad Arguments for New Order transaction
	int w_id = TpccWareHouse::getRandomWareHouse();
	int d_id = TpccDistrict::getRandomDistrict();
	int c_id = TpccCustomer::getRandomCustomer();

	//Create Order lines between 1 to 10
	int orderLines = abs(Logger::getCurrentMicroSec())%10 + 1;
	std::vector<TpccOrderLine*> orders;
	int isAllLocal = 1;
	for (int i=0 ; i <orderLines ; i++) {
		int itemId = TpccItem::getRandomItem();
		int quantity = TpccItem::getRandomQuantity();
		int supplyWarehouse_no = TpccWareHouse::getRandomWareHouse();
		TpccOrderLine* newOrder = new TpccOrderLine();
		newOrder->OL_I_ID = itemId;
		newOrder->OL_QUANTITY = quantity;
		newOrder->OL_SUPPLY_W_ID = supplyWarehouse_no;
		orders.push_back(newOrder);
		if (w_id != supplyWarehouse_no) {
			isAllLocal = 0;
		}
	}

	LOG_DEBUG("TPCC :NewOrder workLoad: warehouse %d, district %d, customer %d & lines %d\n", w_id, d_id, c_id, orderLines);

	// Now we can execute workLoad
    HYFLOW_ATOMIC_START {
		// In WAREHOUSE table: retrieve W_TAX
		HYFLOW_CHECKPOINT_INIT;

    	std::string warehouseId = TpccWareHouse::getWareHouseId(w_id);
    	HYFLOW_FETCH(warehouseId, true);
    	TpccWareHouse* warehouse = (TpccWareHouse*) HYFLOW_ON_READ(warehouseId);
    	float W_TAX = warehouse->W_TAX;

		// In DISTRICT table: retrieve D_TAX, get and inc D_NEXT_O_ID
    	if (0%(BenchmarkExecutor::getItcpr()) == 0) {
			HYFLOW_CHECKPOINT_HERE;
    	}
		std::string districtId = TpccDistrict::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, false);
		TpccDistrict* district = (TpccDistrict*) HYFLOW_ON_WRITE(districtId);
		float D_TAX = district->D_TAX;
		int o_id = district->D_NEXT_O_ID;
		district->D_NEXT_O_ID = o_id + 1;

		// In CUSTOMER table: retrieve discount, last name, credit status
		if (1%(BenchmarkExecutor::getItcpr()) == 0) {
			HYFLOW_CHECKPOINT_HERE;
		}
		std::string customerId = TpccCustomer::getCustomerId(w_id, d_id, c_id);
		HYFLOW_FETCH(customerId, true);
		TpccCustomer* customer = (TpccCustomer*)HYFLOW_ON_READ(customerId);
		float C_DISCOUNT = customer->C_DISCOUNT;
		std::string C_LAST = customer->C_LAST;
		std::string C_CREDIT = customer->C_CREDIT;

		// Create entries in ORDER and NEW-ORDER
		LOG_DEBUG("TPCC :NewOrder new order %d\n", o_id);
		TpccOrder* order = new TpccOrder(w_id, d_id, o_id);
		LOG_DEBUG("TPCC :NewOrder added order %s\n", order->getId().c_str());
		// Adding new row
		HYFLOW_PUBLISH_OBJECT(order);
		order->O_C_ID = customer->C_ID;
		order->O_CARRIER_ID = 0;
		order->O_ALL_LOCAL = isAllLocal;
		order->O_OL_CNT = orderLines;

		TpccNewOrder* newOrder = new TpccNewOrder(w_id, d_id, o_id);
		LOG_DEBUG("TPCC :NewOrder added new order %s\n", newOrder->getId().c_str());
		HYFLOW_PUBLISH_OBJECT(newOrder);
		double totalAmount = 0;
		for (int i=1 ; i <= orderLines; i++) {
			if (i+1%(BenchmarkExecutor::getItcpr()) == 0) {
				HYFLOW_STORE(&i, i);
				HYFLOW_CHECKPOINT_HERE;
			}

			TpccOrderLine* olArg = orders.at(i-1);
			TpccOrderLine* ol = new TpccOrderLine(w_id, d_id, o_id, i);
			LOG_DEBUG("TPCC :NewOrder added OrderLine %s\n", ol->getId().c_str());
			ol->OL_I_ID = olArg->OL_I_ID;
			ol->OL_QUANTITY = olArg->OL_QUANTITY;
			ol->OL_SUPPLY_W_ID = olArg->OL_SUPPLY_W_ID;
			// For each order line
			std::string itemId = TpccItem::getItemId(ol->OL_I_ID);
			HYFLOW_FETCH(itemId, true);

			TpccItem* item = (TpccItem*) HYFLOW_ON_READ(itemId);
			// Retrieve item info
			int I_PRICE = item->I_PRICE;
			std::string I_NAME = item->I_NAME;
			std::string I_DATA = item->I_DATA;

			// Retrieve stock info, Note: Alex did it differently, he used w_id
			std::string stockId = TpccStock::getStockId(ol->OL_SUPPLY_W_ID, ol->OL_I_ID);
			HYFLOW_FETCH(stockId, false);
			TpccStock* stock = (TpccStock*) HYFLOW_ON_WRITE(stockId);
			float S_QUANTITY = stock->S_QUANTITY;
			std::string S_DIST;
			switch (d_id) {
				case 1 :
					S_DIST = stock->S_DIST_01;
					break;
				case 2 :
					S_DIST = stock->S_DIST_02;
					break;
				case 3 :
					S_DIST = stock->S_DIST_03;
					break;
				case 4 :
					S_DIST = stock->S_DIST_04;
					break;
				case 5 :
					S_DIST = stock->S_DIST_05;
					break;
				case 6 :
					S_DIST = stock->S_DIST_06;
					break;
				case 7 :
					S_DIST = stock->S_DIST_07;
					break;
				case 8 :
					S_DIST = stock->S_DIST_08;
					break;
				case 9 :
					S_DIST = stock->S_DIST_09;
					break;
				case 10 :
					S_DIST = stock->S_DIST_10;
					break;
				default:
					Logger::fatal("TPCC :Invalid District Id used\n");
					break;
			}
			std::string S_DATA = stock->S_DATA;
			if (S_QUANTITY - 10 > ol->OL_QUANTITY) {
					stock->S_QUANTITY = S_QUANTITY - ol->OL_QUANTITY;
			} else { // Fill up the stock
					stock->S_QUANTITY = S_QUANTITY - ol->OL_QUANTITY + 91;
			}
			(stock->S_YTD) += ol->OL_QUANTITY;
			stock->S_CNT_ORDER+=1;

			// If line is remote, inc stock.S_REMOTE_CNT()
			if (ol->OL_W_ID != ol->OL_SUPPLY_W_ID) {
				stock->S_CNT_REMOTE+=1;
			}

			ol->OL_NUMBER = i;
			ol->OL_I_ID = item->I_ID;
			ol->OL_SUPPLY_W_ID = w_id; // TODO: what is the supply warehouse for remote orders?
			ol->OL_AMOUNT = (ol->OL_QUANTITY*I_PRICE)*(1+W_TAX+D_TAX-C_DISCOUNT);
			ol->OL_DELIVERY_D = 0;
			ol->OL_DIST_INFO = S_DIST;

			HYFLOW_PUBLISH_OBJECT(ol);
			totalAmount += ol->OL_AMOUNT;
		}
		LOG_DEBUG("TPCC :NewOrder total amount %d\n", totalAmount);
    } HYFLOW_ATOMIC_END;
    return 0;
}

void TpccOps::payment() {
	// Data Generation
	int w_id = TpccWareHouse::getRandomWareHouse();
	int d_id = TpccDistrict::getRandomDistrict();
	int c_id = TpccCustomer::getRandomCustomer();

	LOG_DEBUG("TPCC :Payment workLoad: warehouse %d, district %d, customer %d\n", w_id, d_id, c_id);
	// TODO: Support customer by Last name
	int c_w_id = w_id;
	int c_d_id = d_id;

	int h_amount =  TpccHistory::getRandomAmount();

	HYFLOW_ATOMIC_START {
		// In WAREHOUSE table
		HYFLOW_CHECKPOINT_INIT;
		std::string warehouseId = TpccWareHouse::getWareHouseId(w_id);
		HYFLOW_FETCH(warehouseId, false);
		TpccWareHouse* warehouse = (TpccWareHouse*) HYFLOW_ON_WRITE(warehouseId);
		warehouse->W_YTD += h_amount;

		// In DISTRICT table
		if (0%(BenchmarkExecutor::getItcpr()) == 0) {
			HYFLOW_CHECKPOINT_HERE;
		}
		std::string districtId = TpccDistrict::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, false);
		TpccDistrict* district = (TpccDistrict*) HYFLOW_ON_WRITE(districtId);
		district->D_YTD += h_amount;

		// In CUSTOMER table
		if (1%(BenchmarkExecutor::getItcpr()) == 0) {
			HYFLOW_CHECKPOINT_HERE;
		}
		std::string customerId = TpccCustomer::getCustomerId(w_id, d_id, c_id);
		HYFLOW_FETCH(customerId, false);
		TpccCustomer* customer = (TpccCustomer*)HYFLOW_ON_WRITE(customerId);
		customer->C_BALANCE -= h_amount;
		customer->C_YTD_PAYMENT += h_amount;
		customer->C_CNT_PAYMENT += 1;
		if (customer->C_CREDIT.compare("CR") == 0) {
				std::string c_data = customer->C_DATA_1;
				if (c_data.length() > 500)
						c_data = c_data.substr(0,500);
				else {
					customer->C_DATA_1.append("-R");
				}
		}

		TpccHistory* history = new TpccHistory(w_id, d_id);
		history->H_W_ID = c_w_id;
		history->H_D_ID = c_d_id;
		history->H_C_ID = c_id;
		std::stringstream dateStream;
		dateStream<<Logger::getCurrentTime();
		history->H_DATE = dateStream.str();
		history->H_AMOUNT = h_amount;
		history->H_DATA = warehouse->W_NAME + "    " + district->D_NAME;
		HYFLOW_PUBLISH_OBJECT(history);
	}HYFLOW_ATOMIC_END;
}

void TpccOps::orderStatus() {
	// Data Generation
	int w_id = TpccWareHouse::getRandomWareHouse();
	int d_id = TpccDistrict::getRandomDistrict();

	// TODO: Support customer by Last name
	LOG_DEBUG("TPCC :OrderStatus workLoad: warehouse %d, district %d\n", w_id, d_id);
	HYFLOW_ATOMIC_START{
		// Different than Alex implementation: In District table
		HYFLOW_CHECKPOINT_INIT;

		std::string districtId = TpccDistrict::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, true);
		TpccDistrict* district = (TpccDistrict*) HYFLOW_ON_READ(districtId);
		int o_id = district->D_NEXT_O_ID - 1;
		LOG_DEBUG("TPCC :OrderStatus Order Id %d\n", o_id);
		if(o_id <= 1) {
			LOG_DEBUG("TPCC :Not a single order created for this district to check Status\n");
		}else {
			if (0%(BenchmarkExecutor::getItcpr()) == 0) {
				HYFLOW_CHECKPOINT_HERE;
			}
			std::string orderId = TpccOrder::getOrderId(w_id, d_id, o_id);
			LOG_DEBUG("TPCC :OrderStatus Got order for %s\n", orderId.c_str());
			HYFLOW_FETCH(orderId, true);
			TpccOrder* order = (TpccOrder*) HYFLOW_ON_READ(orderId);

			//Select all orderLines --> this is inefficient
			float olsum = 0.0;
			for(int orLineNumb=1 ; orLineNumb<=order->O_OL_CNT ; orLineNumb++) {
				HYFLOW_STORE(&orLineNumb, orLineNumb);
				HYFLOW_CHECKPOINT_HERE;
				std::string orderLineId = TpccOrderLine::getOrderLineId(w_id, d_id, o_id, orLineNumb);
				LOG_DEBUG("TPCC :OrderStatus Got orderLine for %s\n", orderLineId.c_str());
				HYFLOW_FETCH(orderLineId, true);

				TpccOrderLine* orderLine = (TpccOrderLine*) HYFLOW_ON_READ(orderLineId);
				olsum += orderLine->OL_AMOUNT;
			}
		}
	}HYFLOW_ATOMIC_END;
}

void TpccOps::delivery() {
	int w_id = TpccWareHouse::getRandomWareHouse();

	LOG_DEBUG("TPCC :Delivery workLoad: warehouse %d\n", w_id);
	for (int d_id = 1; d_id <= 10; d_id++) {
		std::string districtId = TpccDistrict::getDistrictId(w_id, d_id);
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			HYFLOW_FETCH(districtId, true);
			TpccDistrict* district = (TpccDistrict*) HYFLOW_ON_READ(districtId);

			int o_id = district->D_NEXT_O_ID - 1;
			int ld_o_id = district->D_LAST_DELV_O_ID;
			LOG_DEBUG("TPCC :Delivery Order Id %d\n", o_id);
			for (int ord = o_id; (ord <= o_id) && (ord > ld_o_id); ord--) {
				if (o_id <= 1 || ((district->D_NEXT_O_ID - ld_o_id) <= 0)) {
					LOG_DEBUG("TPCC :Not a single order created for %d district to check Status\n", d_id);
					break;
				} else {
					if (0%(BenchmarkExecutor::getItcpr()) == 0) {
						HYFLOW_STORE(&ord, ord);
						HYFLOW_CHECKPOINT_HERE;
					}
					district = (TpccDistrict*) HYFLOW_ON_WRITE(districtId);
					std::string orderId = TpccOrder::getOrderId(w_id, d_id, ord);
					LOG_DEBUG("TPCC :Delivery district %d Got order for %s\n", d_id, orderId.c_str());
					HYFLOW_FETCH(orderId, true);
					TpccOrder* order = (TpccOrder*) HYFLOW_ON_READ(orderId);

					if (1%(BenchmarkExecutor::getItcpr()) == 0) {
						HYFLOW_STORE(&ord, ord);
						HYFLOW_CHECKPOINT_HERE;
					}
					std::string newOrderId = TpccNewOrder::getNewOrderId(w_id, d_id,ord);
					HYFLOW_FETCH(newOrderId, false);
					TpccNewOrder* newOrder =
							(TpccNewOrder*) HYFLOW_ON_WRITE(newOrderId);
					HYFLOW_DELETE_OBJECT(newOrder);

					//Select all orderLines --> this is inefficient
					for (int ordLineNumb = 1; ordLineNumb <= order->O_OL_CNT; ordLineNumb++) {
						if ((ordLineNumb+1)%(BenchmarkExecutor::getItcpr()) == 0) {
							HYFLOW_STORE(&ord, ord);
							HYFLOW_STORE(&ordLineNumb, ordLineNumb);
							HYFLOW_CHECKPOINT_HERE;
						}
						std::string orderLineId =TpccOrderLine::getOrderLineId(w_id, d_id, ord, ordLineNumb);
						LOG_DEBUG("TPCC :Delivery district %d Got orderLine for %s\n", d_id, orderLineId.c_str());
						HYFLOW_FETCH(orderLineId, false);
						TpccOrderLine* orderLine =
								(TpccOrderLine*) HYFLOW_ON_WRITE(orderLineId);
						orderLine->OL_DELIVERY_D = 0;
					}

					std::string customerId =
							TpccCustomer::getCustomerId(w_id, d_id,
									order->O_C_ID);
					HYFLOW_FETCH(customerId, false);
					TpccCustomer* customer =
							(TpccCustomer*) HYFLOW_ON_WRITE(customerId);
					customer->C_CNT_DELIVERY += 1;
				}
			}
			district->D_LAST_DELV_O_ID = district->D_NEXT_O_ID;
		}HYFLOW_ATOMIC_END;
	}
}

void TpccOps::stockLevel() {
	int w_id = TpccWareHouse::getRandomWareHouse();
	int d_id = TpccDistrict::getRandomDistrict();
	float thresh = TpccStock::getRandomThreshold();

	LOG_DEBUG("TPCC :OrderStatus workLoad: warehouse %d, district %d, threshold %f\n", w_id, d_id, thresh);
	int d_next_o_id=0;
	HYFLOW_ATOMIC_START {
		HYFLOW_CHECKPOINT_INIT;

		std::string districtId = TpccDistrict::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, true);
		TpccDistrict* district = (TpccDistrict*) HYFLOW_ON_READ(districtId);
		d_next_o_id = district->D_NEXT_O_ID;

		// Grab all distinct Item Ids
		std::set<int> item_ids;
		if (d_next_o_id == 1) {
			LOG_DEBUG("StockLevel :No orders available in this district\n");
		} else {
			for (int o_id = d_next_o_id - 1;(o_id > 1) && ((d_next_o_id - o_id) < 20); o_id--) {
				if (0%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&o_id, o_id);
					HYFLOW_CHECKPOINT_HERE;
				}
				std::string orderId = TpccOrder::getOrderId(w_id, d_id,	o_id);
				HYFLOW_FETCH(orderId, true);
				TpccOrder* order = (TpccOrder*) HYFLOW_ON_READ(orderId);

				for (int ol_id = 1; ol_id <= order->O_OL_CNT; ol_id++) {
					if (ol_id%(BenchmarkExecutor::getItcpr()) == 0) {
						HYFLOW_STORE(&o_id, o_id);
						HYFLOW_STORE(&ol_id, ol_id);
						HYFLOW_CHECKPOINT_HERE;
					}
					std::string orderLineId = TpccOrderLine::getOrderLineId(
							w_id, d_id, o_id, ol_id);
					HYFLOW_FETCH(orderLineId, true);
					TpccOrderLine* orderLine =
							(TpccOrderLine*) HYFLOW_ON_READ(orderLineId);
					item_ids.insert(orderLine->OL_I_ID);
				}
			}
		}

		int lowStockItemCount = 0;
		for (std::set<int>::iterator item_itr = item_ids.begin();
				item_itr != item_ids.end(); item_itr++) {
			std::string stockId = TpccStock::getStockId(w_id, *item_itr);
			HYFLOW_FETCH(stockId, true);
			TpccStock* stock = (TpccStock*) HYFLOW_ON_READ(stockId);
			if (stock->S_QUANTITY < thresh) {
				lowStockItemCount++;
			}
		}
	}HYFLOW_ATOMIC_END;
}

} /* namespace vt_dstm */
