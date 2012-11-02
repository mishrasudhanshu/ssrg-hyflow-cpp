/*
 * BankAccount.cpp
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

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
#include "BankAccount.h"

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

void BankAccount::checkBalanceAtomic(HyflowObject* self, void *args, HyflowContext* context, uint64_t* balance) {
	std::string* id = (std::string*) args;
	context->fetchObject(*id);

	BankAccount* ba = (BankAccount*)context->onReadAccess(*id);
	*balance = ba->checkBalance();
}

void BankAccount::depositAtomic(HyflowObject* self, void* args, HyflowContext* context, uint64_t* ignore) {
	int money = ((BankArgs*)args)->money;
	std::string id = ((BankArgs*)args)->id2;

	context->fetchObject(id);

	BankAccount* baCurrent = (BankAccount*)context->onReadAccess(id);
	BankAccount* ba = (BankAccount*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->deposit(money);
}

void BankAccount::withdrawAtomic(HyflowObject* self, void* args, HyflowContext* context, uint64_t* ignore) {
	int money = ((BankArgs*)args)->money;
	std::string id = ((BankArgs*)args)->id1;

	context->fetchObject(id);

	BankAccount* baCurrent = (BankAccount*)context->onReadAccess(id);
	BankAccount* ba = (BankAccount*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->withdraw(money);
}

void BankAccount::totalBalanceAtomically(HyflowObject* self, void* bankArgs, HyflowContext* c, uint64_t* balance) {
	Atomic<uint64_t> atomicCheckBalance1, atomicCheckBalance2;
	BankArgs* args= (BankArgs*)bankArgs;
	atomicCheckBalance1.atomically = BankAccount::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicCheckBalance1.execute(NULL, &args->id1, balance);

	atomicCheckBalance2.atomically = BankAccount::checkBalanceAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicCheckBalance2.execute(NULL, &args->id2, balance);
}

void BankAccount::transferAtomically(HyflowObject* self, void* bankArgs, HyflowContext* c, void* ignore) {
	Atomic<uint64_t> atomicWithdraw, atomicDeposit;

	atomicWithdraw.atomically = BankAccount::withdrawAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicWithdraw.execute(NULL, bankArgs, NULL);

	atomicDeposit.atomically = BankAccount::depositAtomic;
	for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++)
		atomicDeposit.execute(NULL, bankArgs, NULL);
}

uint64_t BankAccount::totalBalance(std::string id1, std::string id2) {
	uint64_t balance;
	Atomic<uint64_t> atomicBalance;
	atomicBalance.atomically = BankAccount::totalBalanceAtomically;
	BankArgs baArgs(0, id1, id2);
	atomicBalance.execute(NULL, &baArgs, &balance);
	return balance;
}

void BankAccount::transfer(std::string id1, std::string id2,
		uint64_t money) {
	Atomic<void> atomicTransfer;
	atomicTransfer.atomically = BankAccount::transferAtomically;
	BankArgs baArgs(money, id1, id2);
	atomicTransfer.execute(NULL, &baArgs, NULL);
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
