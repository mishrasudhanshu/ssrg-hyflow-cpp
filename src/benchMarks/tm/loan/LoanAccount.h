/*
 * LoanAccount.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef LOANACCOUNT_H_
#define LOANACCOUNT_H_

#include <stdint.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <vector>

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"
#include "../../../core/helper/BenchMarkArgs.h"
#include "../../../core/helper/BenchMarkReturn.h"

#define AMOUNT 100000

namespace vt_dstm {

class LoanArgs: public BenchMarkArgs {
public:
	uint64_t money;
	bool initiator;
	std::string borrower;
	std::vector<std::string> lenders;

	LoanArgs(int m, std::string account) {
		money = m;
		borrower = account;
	}
	LoanArgs(int m, std::string i1, std::vector<std::string> ld) {
		money= m;
		borrower = i1;
		lenders = ld;
		initiator = false;
	}

	void getClone(BenchMarkArgs** args) {

	}
};

class LoanBalance: public BenchMarkReturn {
public:
	uint64_t money;

	void getClone(BenchMarkReturn **ret) {

	}
};

class LoanAccount: public HyflowObject {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<HyflowObject>(*this);
    	ar & amount;
    }

	uint64_t amount;

	void setAmount(uint64_t amount);

	uint64_t checkBalanceInternal();
	void depositInternal(uint64_t money);
	void withdrawInternal(uint64_t money);

	static void checkBalanceAtomic(HyflowObject* self, BenchMarkArgs *args, HyflowContext* c, BenchMarkReturn* balance);
	static void depositAtomic(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static void withdrawAtomic(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);

	static uint64_t checkBalance(std::string account);
	static void deposit(uint64_t money, std::string account);
	static void withdraw(uint64_t money, std::string account);

	static void sumAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* balance);
	static void borrowAtomically(HyflowObject* self, BenchMarkArgs* args, HyflowContext* c, BenchMarkReturn* ignore);
	static double getRandom();
public:
	LoanAccount() {};
	LoanAccount(uint64_t amount, const std::string & Id);
	LoanAccount(uint64_t amount, const std::string & Id, int version);
	virtual ~LoanAccount();

	static uint64_t sum(std::vector<std::string> lenders);
	static void borrow(std::string borrower, std::vector<std::string> lenders, uint64_t money, bool isInit);
	void print();
	void getClone(HyflowObject **obj);
	static void checkSanity(std::string* ids, int objectCount);
	void test();
};


} /* namespace vt_dstm */

#endif /* LOANACCOUNT_H_ */
