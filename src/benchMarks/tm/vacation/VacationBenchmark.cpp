/*
 * VacationBenchmark.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "VacationBenchmark.h"
#include "Vacation.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../BenchmarkExecutor.h"
#include "../../../util/concurrent/ThreadMeta.h"

#define VACATION_CONTENTION 10	//Should be 3*k+1

namespace vt_dstm {

int VacationBenchmark::queryPerTransaction = 10;
int VacationBenchmark::queryRange = 10;
int VacationBenchmark::objectCount=-1;
boost::thread_specific_ptr<HyInteger> VacationBenchmark::objectCreated;

VacationBenchmark::VacationBenchmark() {}

VacationBenchmark::~VacationBenchmark() {
	delete ids;
}

int VacationBenchmark::getOperandsCount() {
	return 2;
}

void VacationBenchmark::readOperation(std::string ids[], int size) {
	processRequest(ACTION_MAKE_RESERVATION);
}

void VacationBenchmark::writeOperation(std::string ids[], int size) {
	int random = abs(Logger::getCurrentMicroSec());
	if (random%2 == 0) {
		processRequest(ACTION_UPDATE_TABLES);
	}else {
		processRequest(ACTION_DELETE_CUSTOMER);
	}
}

int VacationBenchmark::getId() {
	HyInteger* objectCreatedCount = objectCreated.get();
	if (!objectCreatedCount) {
		objectCreated.reset(new HyInteger(0));
		objectCreated.get()->setValue(objectCount+ThreadMeta::getThreadId()*50000);
	}
	objectCreated->increaseValue();
	return objectCreated.get()->getValue();
}

void VacationBenchmark::processRequest(Actions action)
{
	int resourceCount = (objectCount - objectCount/VACATION_CONTENTION);
	int customerCount = objectCount/VACATION_CONTENTION;
	int nodeCount = NetworkManager::getNodeCount();
	switch(action){
		case ACTION_MAKE_RESERVATION: {
			std::string* resourceIds = new std::string[queryPerTransaction];
			for (int queries=0 ; queries<10 ; queries++ ) {
				int randomResource = abs(Logger::getCurrentMicroSec());
				int object = randomResource%resourceCount;
				std::stringstream idStream;
				idStream << object%nodeCount <<"-"<< object;
				resourceIds[queries] = idStream.str();
			}
			int randomCustomer = abs(Logger::getCurrentMicroSec());
			int customer = (randomCustomer%customerCount) + resourceCount;
			std::stringstream custStream;
			custStream<<(customer%nodeCount)<<"-"<<customer;
			std::string customerId = custStream.str();
			Vacation::makeReservation(customerId, resourceIds, queryPerTransaction);
			delete[] resourceIds;
		}
		break;
		case ACTION_DELETE_CUSTOMER:{
			int randomCustomer = abs(Logger::getCurrentMicroSec());
			int customer = (randomCustomer%customerCount) + resourceCount;
			std::stringstream custStream;
			custStream<<(customer%nodeCount)<<"-"<<customer;
			std::string customerId = custStream.str();
			Vacation::deleteCustomer(customerId);
		}
		break;
		case ACTION_UPDATE_TABLES:{
			std::string *resourceIds = new std::string[queryPerTransaction];
			int *prices = new int[queryPerTransaction];
			for (int queries=0 ; queries<10 ; queries++ ) {
				int randomResource = abs(Logger::getCurrentMicroSec());
				int object = randomResource%resourceCount;
				std::stringstream idStream;
				idStream << object%nodeCount <<"-"<< object;
				resourceIds[queries] = idStream.str();
				prices[queries] = queries+10;
			}
			Vacation::updateOffers(resourceIds, prices, queryPerTransaction);
			delete[] resourceIds;
			delete[] prices;
		}
		break;
	}
}

void VacationBenchmark::checkSanity() {
	sleep(2);
	BankAccount::checkSanity(ids, objectCount);
}

std::string* VacationBenchmark::createLocalObjects(int objCount) {
	if (objCount < 10) {
		objCount = 10;
	}
	objectCount = objCount;
	ids = new std::string [objCount];
	int nodeCount =  NetworkManager::getNodeCount();
	int nodeId = NetworkManager::getNodeId();
	// Create car, flights, hotel rooms and customers
	for(int i=0; i<objCount ; i++){
		std::ostringstream idStream;
		idStream << i%nodeCount <<"-"<< i;
		ids[i] = idStream.str();
		if((i % nodeCount)== nodeId ){
			LOG_DEBUG("Created locally object %d\n", i);

			if ( i >= (objectCount - objectCount/VACATION_CONTENTION) ) {	//10th part will be customer
				LOG_DEBUG("VB :Creating Customer object%s\n", ids[i].c_str());
				Customer cust(ids[i]);
				DirectoryManager::registerObjectWait(&cust, 0);
			}else if ( i%VACATION_CONTENTION < VACATION_CONTENTION/3 ) { //1-3 will be car
				LOG_DEBUG("VB :Creating Car object%s\n", ids[i].c_str());
				Resource res(ids[i], i+1, VACATION_CAR);
				DirectoryManager::registerObjectWait(&res, 0);
			}else if ( i%VACATION_CONTENTION < 2*(VACATION_CONTENTION/3) ) {	//4-6 will be flight
				LOG_DEBUG("VB :Creating flight object%s\n", ids[i].c_str());
				Resource res(ids[i], i+1, VACATION_FLIGHT);
				DirectoryManager::registerObjectWait(&res, 0);
			}else if ( i%VACATION_CONTENTION < VACATION_CONTENTION ) {	//7-9 rooms
				LOG_DEBUG("VB :Creating room object%s\n", ids[i].c_str());
				Resource res(ids[i], i+1, VACATION_ROOM);
				DirectoryManager::registerObjectWait(&res, 0);
			}
		}
	}
	return ids;
}

} /* namespace vt_dstm */
