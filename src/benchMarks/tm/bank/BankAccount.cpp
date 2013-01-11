/*
 * BankAccount.cpp
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#include "BankAccount.h"
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <vector>
#include <inttypes.h>
#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../core/helper/Atomic.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/logging/Logger.h"
#include "../../BenchmarkExecutor.h"
#include "../../../core/helper/CheckPointProvider.h"

namespace vt_dstm {

BankAccount::BankAccount(uint64_t amnt, const std::string & Id)
{
	hyId = Id;
	amount = amnt;
	hyVersion = 0;

	//TODO: Add register object call in constructor itself: Requires
	// thread local storage of tid, if not defined register, else add to
	// lazy publish set of object
}

BankAccount::BankAccount(uint64_t amnt, const std::string & Id, int v)
{
	hyId = Id;
	amount = amnt;
	hyVersion = v;
}

BankAccount::~BankAccount() {
	LOG_DEBUG("BANK : Delete Bank Account %s\n", hyId.c_str());
}

void BankAccount::setAmount(uint64_t amnt) {
	amount = amnt;
}

uint64_t BankAccount::checkBalance() {
	return amount;
}

void BankAccount::deposit(uint64_t money) {
	amount += money;
}

void BankAccount::withdraw(uint64_t money) {
	amount -= money;
}

void BankAccount::checkBalanceAtomic(HyflowObject* self, BenchMarkArgs *args, HyflowContext* context, BenchMarkReturn* balance) {
	BankArgs* bargs = (BankArgs*) args;
	std::string id = bargs->ids[bargs->select];
	context->fetchObject(id);

	BankAccount* ba = (BankAccount*)context->onReadAccess(id);
	((BankBalance*)balance)->money += ba->checkBalance();
}

void BankAccount::depositAtomic(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* ignore) {
	uint64_t money = ((BankArgs*)args)->money;
	std::string id = ((BankArgs*)args)->ids[1];

	context->fetchObject(id);

	BankAccount* baCurrent = (BankAccount*)context->onReadAccess(id);
	BankAccount* ba = (BankAccount*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->deposit(money);
	LOG_DEBUG("Bank :In %s Money %llu deposit change %llu to %llu\n", id.c_str(), money, baCurrent->amount, ba->amount);
}

void BankAccount::withdrawAtomic(HyflowObject* self, BenchMarkArgs* args, HyflowContext* context, BenchMarkReturn* ignore) {
	uint64_t money = ((BankArgs*)args)->money;
	std::string id = ((BankArgs*)args)->ids[0];

	context->fetchObject(id);

	BankAccount* baCurrent = (BankAccount*)context->onReadAccess(id);
	BankAccount* ba = (BankAccount*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->withdraw(money);
	LOG_DEBUG("Bank :In %s Money %llu withdraw change %llu to %llu\n", id.c_str(), money, baCurrent->amount, ba->amount);
}

void BankAccount::totalBalanceAtomically(HyflowObject* self, BenchMarkArgs* bankArgs, HyflowContext* c, BenchMarkReturn* balance) {
	Atomic atomicCheckBalance1, atomicCheckBalance2;

	LOG_DEBUG("BANK : Call check Balance1\n");
	atomicCheckBalance1.atomically = BankAccount::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
		((BankArgs*)bankArgs)->select = 0;
		atomicCheckBalance1.execute(NULL, bankArgs, balance);
	}

	LOG_DEBUG("BANK : Call check Balance2\n");
	atomicCheckBalance2.atomically = BankAccount::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
		((BankArgs*)bankArgs)->select = 1;
		atomicCheckBalance2.execute(NULL, bankArgs, balance);
	}
}

void BankAccount::transferAtomically(HyflowObject* self, BenchMarkArgs* bankArgs, HyflowContext* c, BenchMarkReturn* ignore) {
	Atomic atomicWithdraw, atomicDeposit;

	LOG_DEBUG("BANK :Call Withdraw\n");
	atomicWithdraw.atomically = BankAccount::withdrawAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
		atomicWithdraw.execute(NULL, bankArgs, NULL);
	}

	LOG_DEBUG("BANK :Call Deposit\n");
	atomicDeposit.atomically = BankAccount::depositAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
		atomicDeposit.execute(NULL, bankArgs, NULL);
	}
}

uint64_t BankAccount::totalBalance(std::string id1, std::string id2) {
	BankBalance bBal;
	std::string ids[2] = {id1, id2};
	BankArgs baArgs(0, ids, 2);

	Atomic atomicBalance;
	atomicBalance.atomically = BankAccount::totalBalanceAtomically;
	atomicBalance.execute(NULL, &baArgs, &bBal);

	return bBal.money;
}

void BankAccount::transfer(std::string id1, std::string id2,
		uint64_t money) {
	std::string ids[2] = {id1, id2};
	BankArgs baArgs(money, ids, 2);

	Atomic atomicTransfer;
	atomicTransfer.atomically = BankAccount::transferAtomically;
	atomicTransfer.execute(NULL, &baArgs, NULL);
}

void BankAccount::totalBalanceMultiAtomically(HyflowObject* self, BenchMarkArgs* bankArgs, HyflowContext* c, BenchMarkReturn* balance) {
	BankArgs* bArgs = (BankArgs*)bankArgs;
	BankBalance* bRet = (BankBalance*)balance;
	std::string* ids = bArgs->ids;
	for(int txns=0 ; txns<bArgs->size ; txns+=2 ) {
		bRet->money += totalBalance(ids[txns], ids[txns+1]);
	}
}

void BankAccount::transferMultiAtomically(HyflowObject* self, BenchMarkArgs* bankArgs, HyflowContext* c, BenchMarkReturn* ignore) {
	BankArgs* bArgs = (BankArgs*)bankArgs;
	std::string* ids = bArgs->ids;
	int money = bArgs->money;

	for(int txns=0 ; txns<bArgs->size ; txns+=2 ) {
		transfer(ids[txns], ids[txns+1], money);
	}
}


void BankAccount::totalBalanceMulti(std::string ids[], int size) {
	BankBalance balance;
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=2 ) {
				LOG_DEBUG("BANK :Call CheckBalance in txns %d\n", txns);
				for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
					BankArgs baArgs(0, &ids[txns], 1);
					checkBalanceAtomic(NULL, &baArgs, __context__, &balance);
				}

				if ((txns+1)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}

				LOG_DEBUG("BANK :Call CheckBalance in txns %d\n", txns);
				for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
					BankArgs baArgs(0, &ids[txns+1], 1);
					checkBalanceAtomic(NULL, &baArgs, __context__, &balance);
				}

				if ((txns+2)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		BankArgs baArgs(0, ids, size);
		Atomic atomicTransfer;
		atomicTransfer.atomically = BankAccount::totalBalanceMultiAtomically;
		atomicTransfer.execute(NULL, &baArgs, &balance);
	}
}

void BankAccount::transferMulti(std::string ids[], int size, int money) {
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			HYFLOW_CHECKPOINT_INIT;
			for(int txns=0 ; txns<size ; txns+=2 ) {
				LOG_DEBUG("BANK :Call Withdraw in txns %d\n", txns);
				for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
					BankArgs baArgs(money, &ids[txns], 2);
					withdrawAtomic(NULL, &baArgs, __context__, NULL);
				}

				if ((txns+1)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}

				LOG_DEBUG("BANK :Call Deposit in txns %d\n", txns);
				for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
					BankArgs baArgs(money, &ids[txns], 2);
					depositAtomic(NULL, &baArgs, __context__, NULL);
				}

				if ((txns+2)%(BenchmarkExecutor::getItcpr()) == 0) {
					HYFLOW_STORE(&txns, txns);
					HYFLOW_CHECKPOINT_HERE;
				}
			}
		}HYFLOW_ATOMIC_END;
	}else {
		BankArgs baArgs(money, ids, size);
		Atomic atomicTransfer;
		atomicTransfer.atomically = BankAccount::transferMultiAtomically;
		atomicTransfer.execute(NULL, &baArgs, NULL);
	}
}

void BankAccount::print(){
	std::cout <<"Id "<<hyId<<" amount "<<amount;
}

void BankAccount::getClone(HyflowObject **obj){
	BankAccount *ba = new BankAccount();
	ba->amount = amount;
	this->baseClone(ba);
	*obj = ba;
}

void BankAccount::checkSanity(std::string* ids, int objectCount) {
	uint64_t balance = 0;
	HyflowContext *c = ContextManager::getInstance();
	if (NetworkManager::getNodeId() == 0) {
		c->contextInit();
		for(int i=0; i <objectCount ; i++ ) {
			BankAccount* ba = (BankAccount*)DirectoryManager::locate(ids[i], true, c->getTxnId());
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
void BankAccount::test() {
	// create and open a character archive for output
	std::ofstream ofs("BankAccount", std::ios::out);

	// create class instance
	vt_dstm::BankAccount  b(1000,"0-0");

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << b;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::BankAccount b1;
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
