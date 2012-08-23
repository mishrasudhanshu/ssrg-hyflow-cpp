/*
 * Application.cpp
 *
 *  Created on: Aug 11, 2012
 *      Author: mishras[at]vt.edu
 */

#include <string>

#include "Application.h"
#include "../util/unitTests/unitTests.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"

namespace vt_dstm
{

int Nomain(int argc, char *argv[], char *envp[]) {
	// Read Hyflow Configuration file
	ConfigFile::ConfigFileInit("default.conf");

	// Update Configuration based on command line input
	ConfigFile::UpdateMap();

	// Run unit test if specified
	if(ConfigFile::Value(UNIT_TEST).compare(TRUE)==0) {
		unitTests::tests();
	}

	// Set node Id
	NetworkManager::setNodeId();

	// Initiate Logger : Depends on node Id
	Logger::LoggerInit();

	NetworkManager::NetworkInit();

	// Run Benchmarks


	return 0;
}

}

