/*
 * Logger.cpp
 *
 *  Created on: Aug 9, 2012
 *      Author: mishras[at]vt.edu
 */
#include <iostream>
#include <cstddef>
#include <cstdarg>
#include <string>

#include "Logger.h"
#include "Basic/BasicLogLib.h"
#include "Pantheios/Pantheios.h"
#include "../Definitions.h"
#include "../parser/ConfigFile.h"

namespace vt_dstm
{

Logger* Logger::instance = NULL;
bool Logger::logging = false;

Logger::Logger(LoggerType libType) {
	if (strcmp(ConfigFile::Value(DEBUG).c_str(), TRUE) == 0)
		logging = true;

	if (libType == BASIC)
		logLib = new BasicLogLib();
	else if (libType == PANTHEIOS)
		logLib = new Pantheios();
	else if (libType >= LOGGER_TYPE_MAX || libType <= LOGGER_TYPE_INVALID) {
		logLib = new BasicLogLib();
		std::cerr << "Invalid Logger Option provided Default is used"
				<< std::endl;
	}
}

Logger::~Logger() {
	delete logLib;
}

void Logger::LoggerInit() {
	if (instance == NULL) {
		if (strcmp(ConfigFile::Value(LOGGER).c_str(),PANTHEIOS_LOGGER)==0) {
			instance = new Logger(PANTHEIOS);
		}else if (strcmp(ConfigFile::Value(LOGGER).c_str(),BASIC_LOGGER)==0) {
			instance = new Logger(BASIC);
		}
	}
}

void Logger::LoggerDeinit() {
	delete instance;
}

void Logger::info(const char* str, ...) {
	if (logging) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		instance->logLib->info((char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void Logger::debug(const char* str, ...) {
	if (logging) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		instance->logLib->debug((char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void Logger::warn(const char* str, ...) {
	if (logging) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		instance->logLib->warn((char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void Logger::error(const char* str, ...) {
	if (logging) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		instance->logLib->error((char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void Logger::fatal(const char* str, ...) {
	char buf[200];
	va_list argptr; /* Set up the variable argument list here */
	va_start(argptr, str);
	/* Start up variable arguments */
	vsprintf(buf, str, argptr); /* print the variable arguments to buffer */

	if (logging) {
		instance->logLib->fatal((char const*) buf); /* print the message to stream */
	}
	// By default print logging message to console
	fprintf(stderr, (char const*) buf);

	/* Signify end of processing of variable arguments */
	va_end(argptr);
}

void Logger::result(const char* str, ...) {
	char buf[200];
	va_list argptr; /* Set up the variable argument list here */
	va_start(argptr, str);
	/* Start up variable arguments */
	vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
	instance->logLib->result((char const*) buf); /* print the message to stream */
	/* Signify end of processing of variable arguments */
	va_end(argptr);
}

void Logger::test() {
	std::cout << "\n---Testing Logger---\n" << std::endl;
	std::string st="STRING";
	double db=6.71;
	int i=5;

	Logger::LoggerInit();
	Logger::info("Hello to info %s %f %d\n",st.c_str(),db, i);
	Logger::debug("Hello to debug %s %f %d\n",st.c_str(),db, i);
	Logger::warn("Hello to warning %s %f %d\n",st.c_str(),db, i);
	Logger::error("Hello to error %s %f %d\n",st.c_str(),db, i);
	Logger::fatal("Hello to fatal %s %f %d\n",st.c_str(),db, i);
	Logger::result("Hello to result %s %f %d\n",st.c_str(),db, i);
	Logger::LoggerDeinit();
	std::cout<<"\n...Logger Test Completed...\n"<<std::endl;
}

}
