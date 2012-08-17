/*
 * unitTests.cpp
 *
 *  Created on: Aug 11, 2012
 *      Author: mishras[at]vt.edu
 */
#include <iostream>

#include "unitTests.h"
#include "../logging/Logger.h"
#include "../parser/ConfigFile.h"

namespace vt_dstm {

void unitTests::tests() {
	std::cout << "<<<Running Unit Tests>>>" << std::endl;
	// Test Parser
	ConfigFile::test();

	// Test Logging
	Logger::test();
}

}

