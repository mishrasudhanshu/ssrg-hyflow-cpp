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

BankAccount::~BankAccount() {}

void BankAccount::setAmount(uint64_t amnt) {
	amount = amnt;
}

uint64_t BankAccount::checkBalance() {
	return amount;
}

uint64_t BankAccount::checkBalance(HyflowContext* context) {
	context->beforeReadAccess(this);
	BankAccount* ba = (BankAccount*)context->onReadAccess(this);
	return ba->checkBalance();
}

void BankAccount::deposit(uint64_t money) {
	amount += money;
}

void BankAccount::deposit(uint64_t money, HyflowContext* context) {
	context->beforeReadAccess(this);
	BankAccount* ba = (BankAccount*)context->onWriteAccess(this);
	ba->deposit(money);
}

void BankAccount::withdraw(uint64_t money) {
	amount -= money;
}

void BankAccount::withdraw(uint64_t money, HyflowContext* context) {
	context->beforeReadAccess(this);
	BankAccount* ba = (BankAccount*)context->onWriteAccess(this);
	ba->withdraw(money);
}

uint64_t BankAccount::totalBalance(std::string id1, std::string id2, HyflowContext* c, HyflowObjectFuture& of1, HyflowObjectFuture& of2) {
	DirectoryManager::locateAsync(id1, true, c->getTxnId(), of1);
	DirectoryManager::locateAsync(id2, true, c->getTxnId(), of2);
	try{
		long balance = 0;
		BankAccount* account1 = NULL;
		account1 = (BankAccount*)of1.waitOnObject();
		balance += account1->checkBalance(c);

		try {
			BankAccount* account2 = (BankAccount*)of2.waitOnObject();
			balance += account2->checkBalance(c);
		}catch(TransactionException & e){
			throw e;
		} catch (...) {
			throw;
		}
		return balance;
	} catch(TransactionException & e){
		throw e;
	} catch (...) {
		throw;
	}
	return 0;
}

uint64_t BankAccount::totalBalance(std::string id1, std::string id2) {
	uint64_t result = 0;
	for (int i = 0; i < 0x7fffffff; i++) {
		bool commit = true;
		HyflowContext* c = ContextManager::getInstance();
		HyflowObjectFuture of1(id1, true, c->getTxnId());
		HyflowObjectFuture of2(id2, true, c->getTxnId());
		try {
			result = totalBalance(id1, id2, c, of1, of2);
		} catch (...) {
			try {
				ContextManager::cleanInstance(c);
				throw;
			}catch (TransactionException & ex) {
				ex.print();
				commit = false;
			} catch (std::string & s) {
				Logger::fatal("%s\n",s.c_str());
				throw;
			}
		}
		if (commit) {
			try {
				c->commit();
				LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");
				ContextManager::cleanInstance(c);
			} catch(...) {
				try{
					ContextManager::cleanInstance(c);
					throw;
				}catch (TransactionException & ex) {
					ex.print();
					continue;
				} catch (std::string & s) {
					Logger::fatal("%s\n",s.c_str());
				}
			}
			return result;
		}
	}
	throw new TransactionException("Failed to commit the transaction in the defined retries.");
	return 0;
}

void BankAccount::transfer(std::string id1, std::string id2,
		uint64_t money, HyflowContext* c, HyflowObjectFuture& of1, HyflowObjectFuture& of2) {
	try{
		DirectoryManager::locateAsync(id1, true, c->getTxnId(), of1);
		DirectoryManager::locateAsync(id2, true, c->getTxnId(), of2);

		BankAccount* account1 = (BankAccount*)of1.waitOnObject();
		LOG_DEBUG("BANK : Account %s amount %llu version %d\n", account1->hyId.c_str(), account1->amount, account1->getVersion());
		account1->withdraw(money, c);

		try {
			BankAccount* account2 = (BankAccount*)of2.waitOnObject();
			LOG_DEBUG("BANK : Account %s amount %llu version %d\n", account2->hyId.c_str(), account2->amount, account2->getVersion());
			account2->deposit(money, c);
		}catch(TransactionException & e){
			throw e;
		} catch (...) {
			throw;
		}
		return;
	} catch(TransactionException & e){
		throw e;
	} catch (...) {
		throw;
	}
	return;
}


void BankAccount::transfer(std::string id1, std::string id2,
		uint64_t money) {
	for (int i = 0; i < 0x7fffffff; i++) {
		bool commit = true;
		HyflowContext* c = ContextManager::getInstance();
		// FIXME: Set Object within the context
		HyflowObjectFuture of1(id1, true, c->getTxnId()), of2(id2, true, c->getTxnId());
		try {
			transfer(id1, id2, money, c, of1, of2);
		}catch (...) {
			try {
				ContextManager::cleanInstance(c);
				throw;
			} catch (TransactionException & ex) {
				ex.print();
				commit = false;
			} catch (std::string & s) {
				Logger::fatal("%s\n",s.c_str());
			}
		}
		if (commit) {
			try {
				c->commit();
				LOG_DEBUG("++++++++++Transaction Successful ++++++++++\n");
				ContextManager::cleanInstance(c);
			} catch(...) {
				try{
					ContextManager::cleanInstance(c);
					throw;
				} catch (TransactionException & ex) {
					ex.print();
					continue;
				} catch (std::string & s) {
					Logger::fatal("%s\n",s.c_str());
				}
			}
			return;
		}
	}
	throw new TransactionException("Failed to commit the transaction in the defined retries.");
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
	if (NetworkManager::getNodeId() == 0) {
		HyflowContext *c = ContextManager::getInstance();
		for(int i=0; i <objectCount ; i++ ) {
			BankAccount* ba = (BankAccount*)DirectoryManager::locate(ids[i], true, c->getTxnId());
			balance += ba->checkBalance();
		}
		uint64_t expected = 10000*objectCount;
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
