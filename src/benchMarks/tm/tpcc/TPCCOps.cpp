/*
 * TPCCOps.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TPCCOps.h"
#include "../../../core/helper/Atomic.h"
#include "Order.h"
#include "District.h"
#include "Item.h"
#include "NewOrder.h"
#include "OrderLine.h"
#include "Stock.h"
#include "TCustomer.h"
#include "TPCCWareHouse.h"

#define TPCC_CONSTANT 10

namespace vt_dstm {

TPCC_Ops::TPCC_Ops() {}

TPCC_Ops::~TPCC_Ops() {}

int NonURand(int A, int x, int y) {
	return ((abs(Logger::getCurrentMicroSec())%A)|(x+abs(Logger::getCurrentMicroSec())%(y-x)+TPCC_CONSTANT)%(y-x+1))+x;
}


double TPCC_Ops::newOrder() {
	//Create WorkLoad Arguments for New Order transaction
	int w_id = TPCCWareHouse::getRandomWareHouse();
	int d_id = District::getRandomDistrict();
	int c_id = TCustomer::getRandomCustomer();

	//Create Order lines between 1 to 10
	int orderLines = abs(Logger::getCurrentMicroSec())%10 + 1;
	std::vector<OrderLine*> orders;
	int isAllLocal = 1;
	for (int i=0 ; i <orderLines ; i++) {
		int itemId = Item::getRandomItem();
		int quantity = Item::getRandomQuantity();
		int supplyWarehouse_no = TPCCWareHouse::getRandomWareHouse();
		OrderLine* newOrder = new OrderLine(itemId, quantity, supplyWarehouse_no);
		orders.push_back(newOrder);
		if (w_id != supplyWarehouse_no) {
			isAllLocal = 0;
		}
	}

	// Now we can execute workLoad
    HYFLOW_ATOMIC_START {
		// In WAREHOUSE table: retrieve W_TAX
    	std::string warehouseId = TPCCWareHouse::getWareHouseId(w_id);
    	HYFLOW_FETCH(warehouseId, true);
    	TPCCWareHouse* warehouse = (TPCCWareHouse*) HYFLOW_ON_READ(warehouseId);
    	float W_TAX = warehouse->W_TAX;


		// In DISTRICT table: retrieve D_TAX, get and inc D_NEXT_O_ID
		std::string districtId = District::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, false);
		District* district = (District*) HYFLOW_ON_WRITE(districtId);
		float D_TAX = district->D_TAX;
		int o_id = district->D_NEXT_O_ID;
		district->D_NEXT_O_ID = o_id + 1;

		// In CUSTOMER table: retrieve discount, last name, credit status
		std::string customerId = TCustomer::getCustomerId(w_id, d_id, c_id);
		HYFLOW_FETCH(customerId, true);
		TCustomer* customer = (TCustomer*)HYFLOW_ON_READ(customerId);
		float C_DISCOUNT = customer->C_DISCOUNT;
		std::string C_LAST = customer->C_LAST;
		std::string C_CREDIT = customer->C_CREDIT;

		// Create entries in ORDER and NEW-ORDER
		Order* order = new Order(w_id, d_id, o_id);
		// Adding new row
		HYFLOW_PUBLISH_OBJECT(order);
		order->O_C_ID = customer->C_ID;
		order->O_CARRIER_ID = 0;
		order->O_ALL_LOCAL = isAllLocal;
		order->O_OL_CNT = orderLines;

		NewOrder* newOrder = new NewOrder(w_id, d_id, o_id);
		HYFLOW_PUBLISH_OBJECT(newOrder);
		double totalAmount = 0;
		for (int i=1 ; i <= orderLines; i++) {
				OrderLine* ol = orders.at(i-1);
				// For each order line
				std::string itemId = Item::getItemId(ol->OL_I_ID);
				HYFLOW_FETCH(itemId, true);

				Item* item = (Item*) HYFLOW_ON_READ(itemId);
				// Retrieve item info
				int I_PRICE = item->I_PRICE;
				int I_NAME = item->I_NAME;
				int I_DATA = item->I_DATA;

				// Retrieve stock info, Note: Alex did it wrong, he used w_id
				std::string stockId = Stock::getStockId(ol->OL_SUPPLY_W_ID, ol->OL_I_ID);
				HYFLOW_FETCH(stockId, false);
				Stock* stock = (Stock*) HYFLOW_ON_WRITE(stockId);
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
						stock->S_QUANTITY() = S_QUANTITY - ol->OL_QUANTITY;
				} else {
						stock->S_QUANTITY() = S_QUANTITY - ol->OL_QUANTITY + 91;
				}
				stock->S_YTD = stock->S_YTD + ol->OL_QUANTITY;
				stock->S_CNT_ORDER ++;

				// TODO: If line is remote, inc stock.S_REMOTE_CNT()
				if (ol->OL_W_ID != ol->OL_SUPPLY_W_ID) {
					stock->S_CNT_REMOTE++;
				}

				ol->OL_NUMBER = i;
				ol->OL_I_ID = item->I_ID;
				ol->OL_SUPPLY_W_ID = w_id; // TODO: what is the supply warehouse for remote orders?
				ol->OL_AMOUNT = ol->OL_QUANTITY*I_PRICE;
				ol->OL_DELIVERY_D = 0;
				ol->OL_DIST_INFO = S_DIST;

				HYFLOW_PUBLISH_OBJECT(ol);
				totalAmount += ol->OL_AMOUNT;
		}
    } HYFLOW_ATOMIC_END;
}

void TPCC_Ops::payment() {
	// Data Generation
	int w_id = TPCCWareHouse::getRandomWareHouse();
	int d_id = District::getRandomDistrict();
	int c_id = TCustomer::getRandomCustomer();

	// TODO: Support customer by Last name
	int c_w_id = w_id;
	int c_d_id = d_id;

	int h_amount =  TpccHistory::getRandomAmount();

	HYFLOW_ATOMIC_START {
		// In WAREHOUSE table
		std::string warehouseId = TPCCWareHouse::getWareHouseId(w_id);
		HYFLOW_FETCH(warehouseId, false);
		TPCCWareHouse* warehouse = (TPCCWareHouse*) HYFLOW_ON_WRITE(warehouseId);
		warehouse->W_YTD += h_amount;

		// In DISTRICT table
		std::string districtId = District::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, false);
		District* district = (District*) HYFLOW_ON_WRITE(districtId);
		district->D_YTD += h_amount;

		// In CUSTOMER table
		std::string customerId = TCustomer::getCustomerId(w_id, d_id, c_id);
		HYFLOW_FETCH(customerId, false);
		TCustomer* customer = (TCustomer*)HYFLOW_ON_WRITE(customerId);
		customer->C_BALANCE -= h_amount;
		customer->C_YTD_PAYMENT += h_amount;
		customer->C_CNT_PAYMENT += 1;
		if (customer->C_CREDIT.compare("BC") == 0) {
				std::string c_data = customer->C_DATA_1;
				if (c_data.length > 500)
						c_data = c_data.substr(0,500);
				else {
					customer->C_DATA_1.append("-RandomStuff");
				}
		}

		TpccHistory* history = new TpccHistory();
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

void TPCC_Ops::orderStatus() {
	// Data Generation
	int w_id = TPCCWareHouse::getRandomWareHouse();
	int d_id = District::getRandomDistrict();
	int c_id = TCustomer::getRandomCustomer();

	// TODO: Support customer by Last name

	HYFLOW_ATOMIC_START{
		// Different than Alex implementation: In District table
		std::string districtId = District::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, true);
		District* district = (District*) HYFLOW_ON_READ(districtId);
		int o_id = district->D_NEXT_O_ID - 1;
		if(o_id < 1) {
			LOG_DEBUG("TPCC :Not a single order created for this district to check Status\n");
		}else {
			std::string orderId = Order::getOrderId(w_id, d_id, o_id);
			HYFLOW_FETCH(orderId, true);
			Order* order = (Order*) HYFLOW_ON_READ(orderId);

			//Select all orderlines --> this is inefficient
			float olsum = 0.0;
			for(int i=1 ; i<=order->O_OL_CNT ; i++) {
				std::string orderLineId = OrderLine::getOrderLineId(w_id, d_id, o_id, i);
				HYFLOW_FETCH(orderLineId, true);
				OrderLine* orderLine = (OrderLine*) HYFLOW_ON_READ(orderLineId);
				olsum += orderLine->OL_AMOUNT;
			}
		}
	}HYFLOW_ATOMIC_END;
}

void TPCC_Ops::delivery() {
	int w_id = TPCCWareHouse::getRandomWareHouse();
	int o_carrier_id = 1;

	for (int d_id = 1; d_id<=10 ; d_id++ ) {
		HYFLOW_ATOMIC_START {
			std::string districtId = District::getDistrictId(w_id, d_id);
			HYFLOW_FETCH(districtId, false);
			District* district = (District*) HYFLOW_ON_WRITE(districtId);

			int o_id = district->D_LAST_DELV_O_ID;
			if(o_id < 1) {
				LOG_DEBUG("TPCC :Not a single order created for this district to check Status\n");
			}else {
				std::string orderId = Order::getOrderId(w_id, d_id, o_id);
				HYFLOW_FETCH(orderId, true);
				Order* order = (Order*) HYFLOW_ON_READ(orderId);

				std::string newOrderId = NewOrder::getNewOrderId(w_id, d_id, o_id);
				HYFLOW_FETCH(newOrderId, false);
				NewOrder* newOrder = (NewOrder*) HYFLOW_ON_WRITE(newOrderId);
				HYFLOW_DELETE_OBJECT(newOrder);

				//Select all orderLines --> this is inefficient
				float olsum = 0.0;
				for(int i=1 ; i<=order->O_OL_CNT ; i++) {
					std::string orderLineId = OrderLine::getOrderLineId(w_id, d_id, o_id, i);
					HYFLOW_FETCH(orderLineId, false);
					OrderLine* orderLine = (OrderLine*) HYFLOW_ON_WRITE(orderLineId);
					HYFLOW_DELETE_OBJECT(orderLine);
				}

				std::string customerId = TCustomer::getCustomerId(w_id, d_id, order->O_C_ID);
				HYFLOW_FETCH(customerId, false);
				TCustomer* customer = (TCustomer*)HYFLOW_ON_WRITE(customerId);
				customer->C_CNT_DELIVERY+=1;
			}
		}HYFLOW_ATOMIC_END;
	}
}

void TPCC_Ops::stockLevel() {
	int w_id = TPCCWareHouse::getRandomWareHouse();
	int d_id = District::getRandomDistrict();
	float thresh = Stock::getRandomThreshold();

	HYFLOW_ATOMIC_START {
		std::string districtId = District::getDistrictId(w_id, d_id);
		HYFLOW_FETCH(districtId, true);
		District* district = (District*) HYFLOW_ON_READ(districtId);
		int d_next_o_id = district->D_NEXT_O_ID;
	}HYFLOW_ATOMIC_END;

	val itemSet = mutable.Set[Int]()
	var break = false
	var i = 1
	while(i <= 20) {
			if (! break) {
					val orderItemSet = atomic { implicit txn =>
							val o_id = d_next_o_id - 21 + i
							val order = Hyflow.dir.open[TpccOrder](Name.O(w_id, d_id, o_id))

							if (order != null) {
									var orderItemSet = Set[Int]()
									var j = 1
									while (j <= order.O_OL_CNT()) {
											val ol = Hyflow.dir.open[TpccOrderLine](Name.OL(w_id, d_id, o_id, j))
											orderItemSet += ol.OL_I_ID()
											j += 1
									}
									orderItemSet
							} else
									null

					}
					if (orderItemSet != null) {
							itemSet ++= orderItemSet
					} else {
							break = true
					}
			}
			i += 1
	}

	val itemList = itemSet.toList
	var cnt = 0
	i = 1
	while(i <= itemList.length/10) {
			val partialCount = atomic { implicit txn =>
					var res = 0
					var j = 10*i
					while (j <= 10*(i+1)-1) {
							if (j < itemList.length) {
									val stock = Hyflow.dir.open[TpccStock](Name.S(w_id, itemList(j)))
									if (stock.S_QUANTITY() < thresh) {
											res += 1
									}
							}
							j += 1
					}
					res
			}
			cnt += partialCount
			i += 1
	}
	cnt
}

} /* namespace vt_dstm */
