/*
 * DefaultLogLib.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: sudhanshu
 */

#include <ostream>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include "BasicLogLib.h"
#include "../networking/ClusterManager.h"

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

//FIXME: Add the default printings
BasicLogLib::BasicLogLib() {
	infoStr = NULL;
	int e;
	struct stat sb;
	char const *dirName = "log";
	char fileName[20];

	nodeId = ClusterManager::getClusterId();
	e = stat(dirName, &sb);
	if (e != 0) {
		if (errno == ENOENT) {
			std::cout
					<< "The directory does not exist. Creating new directory...\n";
			e = mkdir(dirName, S_IRWXU);
			if (e != 0) {
				std::cerr << "mkdir failed; errno=" << errno<<std::endl;
			}
		}
	}

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s/%d_info.log", dirName, nodeId);
	infoStr = fopen(fileName, "wb+");
	if (infoStr == NULL) {
		std::cerr << "Unable to create info file" << std::endl;
	}

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s/%d_debug.log", dirName, nodeId);
	debugStr = NULL;
	debugStr = fopen(fileName, "wb+");
	if (debugStr == NULL) {
		std::cerr << "Unable to create debug file" << std::endl;
	}

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s/%d_warn.log", dirName, nodeId);
	warnStr = NULL;
	warnStr = fopen(fileName, "wb+");
	if (warnStr == NULL) {
		std::cerr << "Unable to create warn file" << std::endl;
	}

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s/%d_error.log", dirName, nodeId);
	errorStr = NULL;
	errorStr = fopen(fileName, "wb+");
	if (errorStr == NULL) {
		std::cerr << "Unable to create error file" << std::endl;
	}

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s/%d_fatal.log", dirName, nodeId);
	fatalStr = NULL;
	fatalStr = fopen(fileName, "wb+");
	if (fatalStr == NULL) {
		std::cerr << "Unable to create fatal file" << std::endl;
	}

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s/%d_result.log", dirName, nodeId);
	resultStr = NULL;
	resultStr = fopen(fileName, "wb+");
	if (resultStr == NULL) {
		std::cerr << "Unable to create result file" << std::endl;
	}
}

BasicLogLib::~BasicLogLib() {
	if (infoStr)
		fclose(infoStr);
	if (debugStr)
		fclose(debugStr);
	if (warnStr)
		fclose(warnStr);
	if (errorStr)
		fclose(errorStr);
	if (fatalStr)
		fclose(fatalStr);
	if (resultStr)
		fclose(resultStr);
}

void BasicLogLib::info(char const* str, ...) {
	if (infoStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(infoStr, "thread %lld :%.5f :", (long long) pthread_self(), getCurrentTime());
		fprintf(infoStr, (char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void BasicLogLib::debug(char const* str, ...) {
	if (debugStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(debugStr, "thread %lld :%.5f :", (long long) pthread_self(), getCurrentTime());
		fprintf(debugStr, (char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void BasicLogLib::warn(char const* str, ...) {
	if (warnStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(warnStr, "thread %lld :%.5f :", (long long) pthread_self(), getCurrentTime());
		fprintf(warnStr, (char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void BasicLogLib::error(char const* str, ...) {
	if (errorStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(errorStr, "thread %lld :%.5f :", (long long) pthread_self(), getCurrentTime());
		fprintf(errorStr, (char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void BasicLogLib::fatal(char const* str, ...) {
	if (fatalStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(fatalStr, "thread %lld :%.5f :", (long long) pthread_self());
		fprintf(fatalStr, (char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

void BasicLogLib::result(char const* str, ...) {
	if (resultStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(resultStr, "thread %lld :%.5f :", (long long) pthread_self());
		fprintf(resultStr, (char const*) buf); /* print the message to stream */
		/* Signify end of processing of variable arguments */
		va_end(argptr);
	}
}

double BasicLogLib::getCurrentTime(){
	timeval tv;
	gettimeofday (&tv, NULL);
	return tv.tv_sec+0.000001*tv.tv_usec;
}
#ifdef USE_NAMESPACE
}
#endif
