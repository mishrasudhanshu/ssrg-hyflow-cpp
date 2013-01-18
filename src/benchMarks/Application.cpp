/*
 * Application.cpp
 *
 *  Created on: Aug 11, 2012
 *      Author: mishras[at]vt.edu
 */

#include <string>
#include <iostream>
#include "Application.h"
#include "../util/unitTests/unitTests.h"
#include "../util/Definitions.h"
#include "../util/parser/ConfigFile.h"
#include "../util/logging/Logger.h"
#include "../util/networking/NetworkManager.h"
#include "../core/directory/DirectoryManager.h"
#include "../core/context/ContextManager.h"
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

	LOG_DEBUG("APP :Initiating the Network\n");
	NetworkManager::NetworkInit();

	LOG_DEBUG("APP :Initiating the Directory and Context Managers\n");
	DirectoryManager::DirectoryManagerInit();
	ContextManager::ContextManagerInit();

	// Run Benchmarks
	LOG_DEBUG("APP :Initiating the Benchmark Execution\n");
	LOG_DEBUG("APP :....\n");
	sleep(2);
	if (BenchmarkExecutor::executeThreads()) {
		sleep(2);
		// TODO: Add similar shutdown functions for directory and context manager
		NetworkManager::NetworkShutdown();

		//	char* dum = new char[10];	// Used to detect any heap corruption
		LOG_DEBUG("APP :All Done !!!\n");
		// Just to make sure any last time message are printed: for debugging
		std::cerr<<"...."<<std::endl;
	}else {
		std::cerr<<"....!!"<<std::endl;
	}


	return 0;
}
