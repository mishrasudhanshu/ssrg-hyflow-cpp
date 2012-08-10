/*
 * Logger.h
 *
 *  Created on: Aug 9, 2012
 *      Author: sudhanshu
 */

#ifndef LOGGER_H_
#define LOGGER_H_
#include<iostream>
using namespace std;

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

/**
 * This class provides the logging interface to HyflowCPP, under the hood
 * any wrapper of logger class can be written.
 */
enum LoggerType {
	LOGGER_TYPE_INVALID = 0, DEFAULT, PANTHEIOS, LOGGER_TYPE_MAX
};

//TODO: Try to Provide a Stream based behaviour using <<
class Logger {
	class AbstractLogLib *logLib;
	// Singleton Instance
	static class Logger *instance;
	Logger(LoggerType libType);
	~Logger();

public:
	static void LoggerInit(LoggerType libType);
	static void LoggerDeinit();
	static void info(char const* str,...);
	static void debug(char const* str,...){};
	static void warn(char const* str,...){};
	static void error(char const* str,...){};
	static void fatal(char const* str,...){};
};

#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* LOGGER_H_ */
