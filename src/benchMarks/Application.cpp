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
#include "../core/directory/DirectoryManager.h"
#include "BenchmarkExecutor.h"

using namespace vt_dstm;
namespace vt_dstm
{

}

int main(int argc, char *argv[], char *envp[]) {
	// Read Hyflow Configuration file
	ConfigFile::ConfigFileInit("default.conf");

	// Update Configuration based on command line input
	ConfigFile::UpdateMap();

	// Set node Id
	NetworkManager::initNode();

	// Run unit test if specified
	if(ConfigFile::Value(UNIT_TEST).compare(TRUE)==0) {
		unitTests::tests();
		return 0;
	}

	// Initiate Logger : Depends on node Id
	Logger::LoggerInit();

	Logger::debug("APP :Initiating the Network\n");
	NetworkManager::NetworkInit();

	Logger::debug("APP :Initiating the Directory Manager\n");
	DirectoryManager::DirectoryManagerInit();

	// Run Benchmarks
	Logger::debug("APP :Initiating the Benchmark Execution\n");
	Logger::debug("APP :....\n");
	sleep(2);
	BenchmarkExecutor::executeThreads();
//	char* dum = new char[10];	// Used to detect any heap corruption
	Logger::debug("APP :All Done !!!\n");

	// Just to make sure any last time message are printed: for debugging
	Logger::debug("APP :....\n");
	sleep(2);
	return 0;
}
