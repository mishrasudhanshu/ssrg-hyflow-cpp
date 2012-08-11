/*
 * Application.cpp
 *
 *  Created on: Aug 11, 2012
 *      Author: sudhanshu
 */

#include <string.h>

#include "Application.h"
#include "../util/unitTests/unitTests.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

int main(int argc, char *argv[], char *envp[]) {
	// Read Hyflow Configuration file
	ConfigFile::ConfigFileInit("default.conf");

	// Update Configuration based on command line input
	ConfigFile::UpdateMap();

	// Run unit test if specified
	if(strcmp(ConfigFile::Value(UNIT_TEST).c_str(), TRUE)==0) {
		unitTests::tests();
	}

	Logger::LoggerInit();

	// Initiate the Network library


	// Run Benchmarks


	return 0;
}

#ifdef USE_NAMESPACE
}
#endif
