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
#include <boost/serialization/base_object.hpp>
#include "../../../core/context/ContextManager.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/directory/DirectoryManager.h"
#include "../../../util/logging/Logger.h"
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

template<class Archive>
void BankAccount::serialize(Archive & ar, const unsigned int version){
	ar & boost::serialization::base_object<HyflowObject>(*this);
	ar & amount;
}

void BankAccount::setAmount(uint64_t amnt) {
	amount = amnt;
}

uint64_t BankAccount::checkBalance() {
	return amount;
}

uint64_t BankAccount::checkBalance(HyflowContext* context) {
	return amount;
}

void BankAccount::deposit(uint64_t money) {
	amount += money;
}

void BankAccount::deposit(uint64_t money, HyflowContext* c) {
	amount += money;
}

void BankAccount::withdraw(uint64_t money) {
	amount -= money;
}

void BankAccount::withdraw(uint64_t money, HyflowContext* c) {
	amount -= money;
}

uint64_t BankAccount::totalBalance(std::string id1, std::string id2, HyflowContext* c) {
	HyflowObjectFuture of1(id1, true, c->getTransactionId());
	HyflowObjectFuture of2(id2, true, c->getTransactionId());
	DirectoryManager::locateAsync(id1, true, c->getTransactionId(), of1);
	DirectoryManager::locateAsync(id2, true, c->getTransactionId(), of2);
	try{

		long balance = 0;
		BankAccount* account1 = (BankAccount*)of1.waitOnObject();
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
	bool commit = true;
	uint64_t result = 0;
	for (int i = 0; i < 0x7fffffff; i++) {
		HyflowContext* c = ContextManager::getInstance();
		try {
			result = totalBalance(id1, id2, c);
		} catch (TransactionException & ex) {
			commit = false;
		} catch (...) {
			throw;
		}
		if (commit) {
			try {
				c->commit();
			} catch (TransactionException & ex) {
				continue;
			} catch(...) {
				throw;
			}
			return result;
		}
	}
	throw new TransactionException("Failed to commit the transaction in the defined retries.");
	return 0;
}

void BankAccount::transfer(std::string id1, std::string id2,
		uint64_t money, HyflowContext* c) {
	try{
		HyflowObjectFuture of1(id1, true, c->getTransactionId()), of2(id2, true, c->getTransactionId());
		DirectoryManager::locateAsync(id1, true, c->getTransactionId(), of1);
		DirectoryManager::locateAsync(id2, true, c->getTransactionId(), of2);

		BankAccount* account1 = (BankAccount*)of1.waitOnObject();
		Logger::debug("BANK : Account %s amount %llu\n", account1->hyId.c_str(), account1->amount);
		account1->withdraw(money, c);

		try {
			BankAccount* account2 = (BankAccount*)of2.waitOnObject();
			Logger::debug("BANK : Account %s amount %llu\n", account2->hyId.c_str(), account2->amount);
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
	bool commit = true;
	for (int i = 0; i < 0x7fffffff; i++) {
		HyflowContext* c = ContextManager::getInstance();
		try {
			transfer(id1, id2, money, c);
		} catch (TransactionException & ex) {
			commit = false;
		} catch (...) {
			throw;
		}
		if (commit) {
			try {
				c->commit();
			} catch (TransactionException & ex) {
				continue;
			} catch(...) {
				throw;
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
	*obj = new BankAccount(this->amount, this->hyId, this->hyVersion);
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
