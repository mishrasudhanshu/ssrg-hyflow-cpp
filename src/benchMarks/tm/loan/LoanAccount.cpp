/*
 * LoanAccount.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#include "LoanAccount.h"
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

#define HYFLOW_LOAN_BRANCHING 2

namespace vt_dstm {

LoanAccount::LoanAccount(uint64_t amnt, const std::string & Id)
{
	hyId = Id;
	amount = amnt;
	hyVersion = 0;

	//TODO: Add register object call in constructor itself: Requires
	// thread local storage of tid, if not defined register, else add to
	// lazy publish set of object
}

LoanAccount::LoanAccount(uint64_t amnt, const std::string & Id, int v)
{
	hyId = Id;
	amount = amnt;
	hyVersion = v;
}

LoanAccount::~LoanAccount() {
	LOG_DEBUG("Loan :Delete Loan Account %s\n", hyId.c_str());
}

void LoanAccount::setAmount(uint64_t amnt) {
	amount = amnt;
}

uint64_t LoanAccount::checkBalanceInternal() {
	return amount;
}

void LoanAccount::depositInternal(uint64_t money) {
	amount += money;
}

void LoanAccount::withdrawInternal(uint64_t money) {
	amount -= money;
}

void LoanAccount::checkBalanceAtomic(HyflowObject* self, void *args, HyflowContext* context, uint64_t* balance) {
	std::string* id = (std::string*) args;
	context->fetchObject(*id);

	LoanAccount* ba = (LoanAccount*)context->onReadAccess(*id);
	(*balance) += ba->checkBalanceInternal();
}

void LoanAccount::depositAtomic(HyflowObject* self, void* args, HyflowContext* context, void* ignore) {
	int money = ((LoanArgs*)args)->money;
	std::string id = ((LoanArgs*)args)->borrower;

	context->fetchObject(id);

	LoanAccount* baCurrent = (LoanAccount*)context->onReadAccess(id);
	LoanAccount* ba = (LoanAccount*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->depositInternal(money);
}

void LoanAccount::withdrawAtomic(HyflowObject* self, void* args, HyflowContext* context, void* ignore) {
	int money = ((LoanArgs*)args)->money;
	std::string id = ((LoanArgs*)args)->borrower;

	context->fetchObject(id);

	LoanAccount* baCurrent = (LoanAccount*)context->onReadAccess(id);
	LoanAccount* ba = (LoanAccount*)context->onWriteAccess(id);

	ba->setAmount(baCurrent->amount);
	ba->withdrawInternal(money);
}

void LoanAccount::sumAtomically(HyflowObject* self, void* loanArgs, HyflowContext* __context__, uint64_t* balance) {
	LoanArgs* args= (LoanArgs*)loanArgs;

	std::vector<std::string> accountNums = args->lenders;

	for (int i = 0; (i < HYFLOW_LOAN_BRANCHING) &&  (!accountNums.empty()); i++) {
		std::string account = accountNums.at(accountNums.size()-1);
		accountNums.pop_back();
		checkBalance(account);
//		checkBalanceAtomic(NULL, &account, __context__, balance);
		LOG_DEBUG("Loan :Call check Balance on %s returned %llu\n", account.c_str(), *balance);
//		LoanArgs newArgs(0, "", accountNums);
//		sumAtomically(NULL, &newArgs, __context__, balance);
		sum(accountNums);
	}
}

uint64_t LoanAccount::checkBalance(std::string account) {
	uint64_t balance=0;
	Atomic<uint64_t> atomicCheck;
	atomicCheck.atomically = LoanAccount::checkBalanceAtomic;
	atomicCheck.execute(NULL, &account, &balance);
	return balance;
}

void LoanAccount::deposit(uint64_t money, std::string account) {
	LoanArgs largs(money,account);
	Atomic<void> atomicDeposit;
	atomicDeposit.atomically = LoanAccount::depositAtomic;
	atomicDeposit.execute(NULL, &largs, NULL);
}

void LoanAccount::withdraw(uint64_t money, std::string account) {
	LoanArgs largs(money,account);
	Atomic<void> atomicWithdraw;
	atomicWithdraw.atomically = LoanAccount::withdrawAtomic;
	atomicWithdraw.execute(NULL, &largs, NULL);
}

void LoanAccount::borrowAtomically(HyflowObject* self, void* loanArgs, HyflowContext* __context__, void* ignore) {
	LoanArgs* args= (LoanArgs*)loanArgs;

	std::vector<std::string> accountNums = args->lenders;

	if(!args->initiator) {
		withdrawAtomic(NULL, args, __context__, NULL);
		LOG_DEBUG("Loan :Call borrow on %s to withdraw %llu\n", args->borrower.c_str(), args->money);
	}

	// Try to grab some withdrawn money from followers
	int borrowAmount = args->money;
	for (int i = 0; (i < HYFLOW_LOAN_BRANCHING) &&  (!accountNums.empty()); i++) {
		std::string account = accountNums.at(accountNums.size()-1);
		accountNums.pop_back();

		bool last = (i==HYFLOW_LOAN_BRANCHING-1 || accountNums.empty());	// is the last one?
		uint64_t loan = last ? borrowAmount : (int)(getRandom()*borrowAmount);

//		LoanArgs newArgs(borrowAmount,account,accountNums);
//		borrowAtomically(NULL, &newArgs,__context__, NULL);
		borrow(account, accountNums, borrowAmount, false);

//		LoanArgs deposArgs(loan, account, accountNums);
//		depositAtomic(NULL, &deposArgs, __context__, NULL);
		deposit(loan, account);

		LOG_DEBUG("Loan :Call Deposit on %s to deposit %llu\n", account.c_str(), loan);
		borrowAmount -= loan;
	}
}

uint64_t LoanAccount::sum(std::vector<std::string> ids) {
	uint64_t balance=0;
	LoanArgs laArgs(0, "", ids);
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
				sumAtomically(NULL, &laArgs, __context__, &balance);
			}
		}HYFLOW_ATOMIC_END;
	}else {
		Atomic<uint64_t> atomicSum;
		atomicSum.atomically = sumAtomically;
		atomicSum.execute(NULL, &laArgs, NULL);
	}
	return balance;
}

void LoanAccount::borrow(std::string id1, std::vector<std::string> ids,
		uint64_t money, bool isInit) {
	LoanArgs laArgs(money, id1, ids);
	laArgs.initiator = isInit;
	if (ContextManager::getNestingModel() == HYFLOW_CHECKPOINTING) {
		HYFLOW_ATOMIC_START {
			//TODO: Re-write Loan in non-recursive approach, to support checkPointing
			for(int i=0 ; i < BenchmarkExecutor::getCalls(); i++) {
				borrowAtomically(NULL, &laArgs, __context__, NULL);
			}
		}HYFLOW_ATOMIC_END;
	}else {
		Atomic<void> atomicBorrow;
		atomicBorrow.atomically = borrowAtomically;
		atomicBorrow.execute(NULL, &laArgs, NULL);
	}
}

void LoanAccount::print(){
	std::cout <<"Id "<<hyId<<" amount "<<amount;
}

void LoanAccount::getClone(HyflowObject **obj){
	LoanAccount *la = new LoanAccount();
	la->amount = amount;
	this->baseClone(la);
	*obj = la;
}

void LoanAccount::checkSanity(std::string* ids, int objectCount) {}

double LoanAccount::getRandom() {
	double random;
	unsigned int seed = abs(Logger::getCurrentMicroSec()%100);
	srand(seed);
    random=(rand()/(double)RAND_MAX) ;
    return random;
}

// Serialisation Test of object
void LoanAccount::test() {
	// create and open a character archive for output
	std::ofstream ofs("LoanAccount", std::ios::out);

	// create class instance
	vt_dstm::LoanAccount  b(1000,"0-0");

	// save data to archive
	{
		boost::archive::text_oarchive oa(ofs);
		// write class instance to archive
		oa << b;
		// archive and stream closed when destructors are called
	}

	// ... some time later restore the class instance to its orginal state
	vt_dstm::LoanAccount b1;
	{
		// create and open an archive for input
		std::ifstream ifs("LoanAccount", std::ios::in);
		boost::archive::text_iarchive ia(ifs);
		// read class state from archive
		ia >> b1;
		// archive and stream closed when destructors are called
		b1.print();
	}
}

} /* namespace vt_dstm */
