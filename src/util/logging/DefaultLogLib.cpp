/*
 * DefaultLogLib.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: sudhanshu
 */
#include "DefaultLogLib.hpp"
#include <ostream>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

//FIXME: Add the fopen result checking
DefaultLogLib::DefaultLogLib() {
	infoStr = NULL;
	int e;
	struct stat sb;
	char const*name = "log";

	e = stat(name, &sb);
	if (e != 0) {
		cerr<<"stat failed"<<endl;
		if (errno == ENOENT) {
			cout<<"The directory does not exist. Creating new directory...\n";
			e = mkdir(name, S_IRWXU);
			if (e != 0) {
				printf("mkdir failed; errno=%d\n", errno);
			}
		}
	}

	infoStr = fopen("log/info.log", "wb+");
	if (infoStr == NULL) {
		cerr << "Unable to create info file" << endl;
	}

	debugStr = NULL;
	debugStr = fopen("log/debug.log", "wb+");
	if (debugStr == NULL) {
		cerr << "Unable to create debug file" << endl;
	}

	warnStr = NULL;
	warnStr = fopen("log/warn.log", "wb+");
	if (warnStr == NULL) {
		cerr << "Unable to create warn file" << endl;
	}

	errorStr = NULL;
	errorStr = fopen("log/error.log", "wb+");
	if (errorStr == NULL) {
		cerr << "Unable to create error file" << endl;
	}

	fatalStr = NULL;
	fatalStr = fopen("log/fatal.log", "wb+");
	if (fatalStr == NULL) {
		cerr << "Unable to create fatal file" << endl;
	}
}

DefaultLogLib::~DefaultLogLib() {
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
}

void DefaultLogLib::info(char const* str, ...) {
	if (infoStr) {
		char buf[200];
		va_list argptr; /* Set up the variable argument list here */
		va_start(argptr, str);
		/* Start up variable arguments */
		vsprintf(buf, str, argptr); /* print the variable arguments to buffer */
		fprintf(infoStr, (char const*) buf); /* print the message to stream */
		fprintf(infoStr, "Info:%ll", (long long) pthread_self());
		va_end(argptr);
		/* Signify end of processing of variable arguments */
	}
}

#ifdef USE_NAMESPACE
}
#endif
