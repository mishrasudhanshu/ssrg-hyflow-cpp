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
#include "../../../core/helper/CheckPointProvider.h"

namespace vt_dstm {

ReservationInfo::ReservationInfo(uint64_t amnt, const std::string & Id)
{
	hyId = Id;
	amount = amnt;
	hyVersion = 0;

	//TODO: Add register object call in constructor itself: Requires
	// thread local storage of tid, if not defined register, else add to
	// lazy publish set of object
}

ReservationInfo::ReservationInfo(uint64_t amnt, const std::string & Id, int v)
{
	hyId = Id;
	amount = amnt;
	hyVersion = v;
}

ReservationInfo::~ReservationInfo() {
	LOG_DEBUG("BANK : Delete Bank Account %s\n", hyId.c_str());
}

void ReservationInfo::setAmount(uint64_t amnt) {
	amount = amnt;
}

uint64_t ReservationInfo::checkBalance() {
	return amount;
}

void ReservationInfo::deposit(uint64_t money) {
	amount += money;
}

void ReservationInfo::withdraw(uint64_t money) {
	amount -= money;
}

void ReservationInfo::checkBalanceAtomic(HyflowObject* self, void *args, HyflowContext* context, uint64_t* balance) {
	std::string* id = (std::string*) args;
	context->fetchObject(*id);

	ReservationInfo* ba = (ReservationInfo*)context->onReadAccess(*id);
	(*balance) += ba->checkBalance();
}

void ReservationInfo::depositAtomic(HyflowObject* self, void* args, HyflowContext* context, uint64_t* ignore) {
	int money = ((ReservationInfoArgs*)args)->money;
	std::string id = ((ReservationInfoArgs*)args)->id2;

	context->fetchObject(id);

	ReservationInfo* baCurrent = (ReservationInfo*)context->onReadAccess(id);
	ReservationInfo* ba = (ReservationInfo*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->deposit(money);
}

void ReservationInfo::withdrawAtomic(HyflowObject* self, void* args, HyflowContext* context, uint64_t* ignore) {
	int money = ((ReservationInfoArgs*)args)->money;
	std::string id = ((ReservationInfoArgs*)args)->id1;

	context->fetchObject(id);

	ReservationInfo* baCurrent = (ReservationInfo*)context->onReadAccess(id);
	ReservationInfo* ba = (ReservationInfo*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->withdraw(money);
}

void ReservationInfo::totalBalanceAtomically(HyflowObject* self, void* bankArgs, HyflowContext* c, uint64_t* balance) {
	Atomic<uint64_t> atomicCheckBalance1, atomicCheckBalance2;
	ReservationInfoArgs* args= (ReservationInfoArgs*)bankArgs;

	LOG_DEBUG("BANK : Call check Balance1\n");
	atomicCheckBalance1.atomically = ReservationInfo::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicCheckBalance1.execute(NULL, &args->id1, balance);

	LOG_DEBUG("BANK : Call check Balance2\n");
	atomicCheckBalance2.atomically = ReservationInfo::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicCheckBalance2.execute(NULL, &args->id2, balance);
}

void ReservationInfo::transferAtomically(HyflowObject* self, void* bankArgs, HyflowContext* c, void* ignore) {
	Atomic<uint64_t> atomicWithdraw, atomicDeposit;

	LOG_DEBUG("BANK :Call Withdraw\n");
	atomicWithdraw.atomically = ReservationInfo::withdrawAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicWithdraw.execute(NULL, bankArgs, NULL);

	LOG_DEBUG("BANK :Call Deposit\n");
	atomicDeposit.atomically = ReservationInfo::depositAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicDeposit.execute(NULL, bankArgs, NULL);
}

uint64_t ReservationInfo::totalBalance(std::string id1, std::string id2) {
	uint64_t balance;
	ReservationInfoArgs baArgs(0, id1, id2);
	// We can not use atomically instrumented class for checkPointing as
	// stack would have unwounded for atomic call.
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			LOG_DEBUG("BANK :Call Withdraw\n");
			for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
				checkBalanceAtomic(NULL, &baArgs.id1, __context__, &balance);
			}
			HYFLOW_CHECKPOINT_HERE;
			LOG_DEBUG("BANK :Call Deposit\n");
			for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
				checkBalanceAtomic(NULL, &baArgs.id2, __context__, &balance);
			}
		}HYFLOW_ATOMIC_END;
	}else {
		Atomic<uint64_t> atomicBalance;
		atomicBalance.atomically = ReservationInfo::totalBalanceAtomically;
		atomicBalance.execute(NULL, &baArgs, &balance);
	}
	return balance;
}

void ReservationInfo::transfer(std::string id1, std::string id2,
		uint64_t money) {
	ReservationInfoArgs baArgs(money, id1, id2);
	// We can not use atomically instrumented class for checkPointing as
	// stack would have unwounded for atomic call.
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			LOG_DEBUG("BANK :Call Withdraw\n");
			for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
				withdrawAtomic(NULL, &baArgs, __context__, NULL);
			}
			HYFLOW_CHECKPOINT_HERE;
			LOG_DEBUG("BANK :Call Deposit\n");
			for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
				depositAtomic(NULL, &baArgs, __context__, NULL);
			}
		}HYFLOW_ATOMIC_END;
	}else {
		Atomic<void> atomicTransfer;
		atomicTransfer.atomically = ReservationInfo::transferAtomically;
		atomicTransfer.execute(NULL, &baArgs, NULL);
	}
}

void ReservationInfo::print(){
	std::cout <<"Id "<<hyId<<" amount "<<amount;
}

void ReservationInfo::getClone(HyflowObject **obj){
	ReservationInfo *ba = new ReservationInfo();
	ba->amount = amount;
	this->baseClone(ba);
	*obj = ba;
}

void ReservationInfo::checkSanity(std::string* ids, int objectCount) {
	uint64_t balance = 0;
	HyflowContext *c = ContextManager::getInstance();
	if (NetworkManager::getNodeId() == 0) {
		c->contextInit();
		for(int i=0; i <objectCount ; i++ ) {
			ReservationInfo* ba = (ReservationInfo*)DirectoryManager::locate(ids[i], true, c->getTxnId());
			balance += ba->checkBalance();
		}
		uint64_t expected = AMOUNT*objectCount;
		if ( expected == balance ) {
			LOG_DEBUG("Sanity check passed...\n");
		} else {
			Logger::fatal("Sanity check failed... expected = %llu & Actual = %llu\n", expected, balance	);
		}
	}
}

// Serialisation Test of object
void ReservationInfo::test() {
	// create and open a character archive for output
	std::ofstream ofs("BankAccount", std::ios::out);

	// create class instance
	vt_dstm::ReservationInfo  b(1000,"0-0");

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << b;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::ReservationInfo b1;
	{
		// create and open an archive for input
		std::ifstream ifs("BankAccount", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> b1;
		// archive and stream closed when destructors are called
		b1.print();
	}
}

} /* namespace vt_dstm */
