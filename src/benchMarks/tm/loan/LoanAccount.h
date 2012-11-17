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

#include "../../../core/HyflowObject.h"
#include "../../../core/context/HyflowContext.h"
#include "../../../core/HyflowObjectFuture.h"

#define AMOUNT 100000

namespace vt_dstm {

class LoanArgs {
public:
	int money;
	std::string id1;
	std::string id2;

	LoanArgs(int m, std::string i1, std::string i2) {
		money= m;
		id1 = i1;
		id2 = i2;
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

	uint64_t checkBalance();
	void deposit(uint64_t money);
	void withdraw(uint64_t money);

	static void checkBalanceAtomic(HyflowObject* self, void *args, HyflowContext* c, uint64_t* balance);
	static void depositAtomic(HyflowObject* self, void* args, HyflowContext* c, uint64_t* ignore);
	static void withdrawAtomic(HyflowObject* self, void* args, HyflowContext* c, uint64_t* ignore);

	static void totalBalanceAtomically(HyflowObject* self, void* args, HyflowContext* c, uint64_t* balance);
	static void transferAtomically(HyflowObject* self, void* args, HyflowContext* c, void* ignore);
public:
	LoanAccount() {};
	LoanAccount(uint64_t amount, const std::string & Id);
	LoanAccount(uint64_t amount, const std::string & Id, int version);
	virtual ~LoanAccount();


	static uint64_t totalBalance(std::string id1, std::string id2);
	static void transfer(std::string fromId, std::string toId, uint64_t money);
	void print();
	void getClone(HyflowObject **obj);
	static void checkSanity(std::string* ids, int objectCount);
	void test();
};


} /* namespace vt_dstm */

#endif /* LOANACCOUNT_H_ */
