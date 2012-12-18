/*
 * BankAccount.h
 *
 *  Created on: Aug 20, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BANKACCOUNT_H_
#define BANKACCOUNT_H_

#include <stdint.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/helper/BenchMarkArgs.h"
#include "../../../core/helper/BenchMarkReturn.h"

#define AMOUNT 100000

namespace vt_dstm {

class BankArgs: public BenchMarkArgs{
public:
	uint64_t money;
	std::string* ids;
	int  size;
	int select;

	BankArgs(int m, std::string* ides, int s) {
		money= m;
		ids = ides;
		size = s;
		select = 0;
	}

	void getClone(BenchMarkArgs** args) {

	}
};

class BankBalance: public BenchMarkReturn {
public:
	uint64_t money;

	BankBalance() {
		money = 0;
	}

	BankBalance(uint64_t balance) {
		money = balance;
	}

	void getClone(BenchMarkReturn** args) {

	}
};

class BankAccount: public HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & amount;
    }

	uint64_t amount;

	void setAmount(uint64_t amount);

	uint64_t checkBalance();
	void deposit(uint64_t money);
	void withdraw(uint64_t money);

	static void checkBalanceAtomic(HyflowObject* self, BenchMarkArgs *args, HyflowContext* c, BenchMarkReturn* balance);
	static void depositAtomic(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void withdrawAtomic(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	static void totalBalanceAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* balance);
	static void transferAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void totalBalanceMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* balance);
	static void transferMultiAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
public:
	BankAccount() {};
	BankAccount(uint64_t amount, const std::string & Id);
	BankAccount(uint64_t amount, const std::string & Id, int version);
	virtual ~BankAccount();

	static uint64_t totalBalance(std::string id1, std::string id2);
	static void transfer(std::string fromId, std::string toId, uint64_t money);
	static void transferMulti(std::string ids[], int size, int money);
	static void totalBalanceMulti(std::string ids[], int size);
	void print();
	void getClone(HyflowObject **obj);
	static void checkSanity(std::string* ids, int objectCount);
	void test();
};


} /* namespace vt_dstm */

#endif /* BANKACCOUNT_H_ */
