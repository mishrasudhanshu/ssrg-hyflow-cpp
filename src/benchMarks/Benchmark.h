/*
 * Benchmark.h
 *
 *  Created on: Aug 23, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include "tm/bank/BankAccount.h"

namespace vt_dstm {

class Benchmark {
public:
	Benchmark();
	virtual ~Benchmark();

	/* TODO: Try to retrieve this using configuration file
	 * This function allows user to register its class so that it can be
	 * registered as HyflowObject derived class in boost::serilization
	 */
    template<class Archive>
	static void registerObjectTypes(Archive & ar){
    	ar.register_type(static_cast<BankAccount*>(NULL));
    }
};

} /* namespace vt_dstm */

#endif /* BENCHMARK_H_ */
