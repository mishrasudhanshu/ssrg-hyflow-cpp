/*
 * Logger.cpp
 *
 *  Created on: Aug 9, 2012
 *      Author: sudhanshu
 */
#include "Logger.hpp"
#include "DefaultLogLib.hpp"
#include "Pantheios.hpp"
#include <iostream>
#include <stddef.h>
#include <stdarg.h>

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

Logger* Logger::instance = NULL;

Logger::Logger(LoggerType libType) {
	if (libType == DEFAULT) {
		logLib = new DefaultLogLib();
	}else if(libType == PANTHEIOS) {
		logLib = new Pantheios();
	}else if(libType > LOGGER_TYPE_MAX || libType < DEFAULT) {
		logLib = new DefaultLogLib();
		cerr<<"Invalid Logger Option provided Default is used";
	}
}

Logger::~Logger(){
	delete logLib;
}

void Logger::LoggerInit(LoggerType libType) {
	if (instance == NULL) {
		instance = new Logger(libType);
	}
}

void Logger::LoggerDeinit(){
	delete instance;
}

void Logger::info(const char* str, ...) {
	char buf[200];
	va_list argptr; /* Set up the variable argument list here */
	va_start(argptr, str); /* Start up variable arguments */
    vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
    instance->logLib->info((char const*)buf); /* print the message to stream */
    va_end(argptr);  /* Signify end of processing of variable arguments */
}

int main(int argc, char **argv) {
	cout<<"Hello Logger"<<endl;
	Logger::LoggerInit(DEFAULT);
	Logger::info("Hello to info");
	Logger::debug("Hello to debug");
	Logger::warn("Hello to warning");
	Logger::error("Hello to error");
	Logger::fatal("Hello to fatal");
	Logger::LoggerDeinit();
}

#ifdef USE_NAMESPACE
}
#endif
