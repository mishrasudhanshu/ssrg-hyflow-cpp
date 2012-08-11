/*
 * unitTests.cpp
 *
 *  Created on: Aug 11, 2012
 *      Author: sudhanshu
 */
#include <iostream>

#include "unitTests.h"
#include "../logging/Logger.h"
#include "../parser/ConfigFile.h"

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

void unitTests::tests() {
	std::cout<<"<<<Running Unit Tests>>>"<<std::endl;
	// Test Parser
	ConfigFile::test();

	// Test Logging
	Logger::test();
}

#ifdef USE_NAMESPACE
}
#endif
