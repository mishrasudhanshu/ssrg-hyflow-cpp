/*
 * TPCC.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TPCC.h"

namespace vt_dstm {

TPCC::TPCC() {
	// TODO Auto-generated constructor stub

}

TPCC::~TPCC() {
	// TODO Auto-generated destructor stub
}

} /* namespace vt_dstm */
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

TPCC::TPCC(uint64_t amnt, const std::string & Id)
{
	hyId = Id;
	amount = amnt;
	hyVersion = 0;

	//TODO: Add register object call in constructor itself: Requires
	// thread local storage of tid, if not defined register, else add to
	// lazy publish set of object
}

TPCC::TPCC(uint64_t amnt, const std::string & Id, int v)
{
	hyId = Id;
	amount = amnt;
	hyVersion = v;
}

TPCC::~TPCC() {
	LOG_DEBUG("BANK : Delete Bank Account %s\n", hyId.c_str());
}

void TPCC::setAmount(uint64_t amnt) {
	amount = amnt;
}

uint64_t TPCC::checkBalance() {
	return amount;
}

void TPCC::deposit(uint64_t money) {
	amount += money;
}

void TPCC::withdraw(uint64_t money) {
	amount -= money;
}

void TPCC::checkBalanceAtomic(HyflowObject* self, void *args, HyflowContext* context, uint64_t* balance) {
	std::string* id = (std::string*) args;
	context->fetchObject(*id);

	TPCC* ba = (TPCC*)context->onReadAccess(*id);
	(*balance) += ba->checkBalance();
}

void TPCC::depositAtomic(HyflowObject* self, void* args, HyflowContext* context, uint64_t* ignore) {
	int money = ((TPCCArgs*)args)->money;
	std::string id = ((TPCCArgs*)args)->id2;

	context->fetchObject(id);

	TPCC* baCurrent = (TPCC*)context->onReadAccess(id);
	TPCC* ba = (TPCC*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->deposit(money);
}

void TPCC::withdrawAtomic(HyflowObject* self, void* args, HyflowContext* context, uint64_t* ignore) {
	int money = ((TPCCArgs*)args)->money;
	std::string id = ((TPCCArgs*)args)->id1;

	context->fetchObject(id);

	TPCC* baCurrent = (TPCC*)context->onReadAccess(id);
	TPCC* ba = (TPCC*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->withdraw(money);
}

void TPCC::totalBalanceAtomically(HyflowObject* self, void* bankArgs, HyflowContext* c, uint64_t* balance) {
	Atomic<uint64_t> atomicCheckBalance1, atomicCheckBalance2;
	TPCCArgs* args= (TPCCArgs*)bankArgs;

	LOG_DEBUG("BANK : Call check Balance1\n");
	atomicCheckBalance1.atomically = TPCC::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicCheckBalance1.execute(NULL, &args->id1, balance);

	LOG_DEBUG("BANK : Call check Balance2\n");
	atomicCheckBalance2.atomically = TPCC::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicCheckBalance2.execute(NULL, &args->id2, balance);
}

void TPCC::transferAtomically(HyflowObject* self, void* bankArgs, HyflowContext* c, void* ignore) {
	Atomic<uint64_t> atomicWithdraw, atomicDeposit;

	LOG_DEBUG("BANK :Call Withdraw\n");
	atomicWithdraw.atomically = TPCC::withdrawAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicWithdraw.execute(NULL, bankArgs, NULL);

	LOG_DEBUG("BANK :Call Deposit\n");
	atomicDeposit.atomically = TPCC::depositAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicDeposit.execute(NULL, bankArgs, NULL);
}

uint64_t TPCC::totalBalance(std::string id1, std::string id2) {
	uint64_t balance;
	TPCCArgs baArgs(0, id1, id2);
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
		atomicBalance.atomically = TPCC::totalBalanceAtomically;
		atomicBalance.execute(NULL, &baArgs, &balance);
	}
	return balance;
}

void TPCC::transfer(std::string id1, std::string id2,
		uint64_t money) {
	TPCCArgs baArgs(money, id1, id2);
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
		atomicTransfer.atomically = TPCC::transferAtomically;
		atomicTransfer.execute(NULL, &baArgs, NULL);
	}
}

void TPCC::print(){
	std::cout <<"Id "<<hyId<<" amount "<<amount;
}

void TPCC::getClone(HyflowObject **obj){
	TPCC *ba = new TPCC();
	ba->amount = amount;
	this->baseClone(ba);
	*obj = ba;
}

void TPCC::checkSanity(std::string* ids, int objectCount) {
	uint64_t balance = 0;
	HyflowContext *c = ContextManager::getInstance();
	if (NetworkManager::getNodeId() == 0) {
		c->contextInit();
		for(int i=0; i <objectCount ; i++ ) {
			TPCC* ba = (TPCC*)DirectoryManager::locate(ids[i], true, c->getTxnId());
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
void TPCC::test() {
	// create and open a character archive for output
	std::ofstream ofs("BankAccount", std::ios::out);

	// create class instance
	vt_dstm::TPCC  b(1000,"0-0");

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << b;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::TPCC b1;
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
