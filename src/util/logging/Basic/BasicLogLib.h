/*
 * DefaultLogLib.h
 *
 *  Created on: Aug 9, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DEFAULTLOGLIB_H_
#define DEFAULTLOGLIB_H_

#include <fstream>
#include <stdint.h>
#include <boost/thread/mutex.hpp>

#include "../AbstractLogLib.h"

namespace vt_dstm
{

class BasicLogLib:public AbstractLogLib {
	FILE *infoStr;
	FILE *debugStr;
	FILE *warnStr;
	FILE *errorStr;
	FILE *fatalStr;
	FILE *resultStr;
	boost::mutex infoGuard;
	boost::mutex debugGuard;
	boost::mutex warnGuard;
	boost::mutex errorGuard;
	boost::mutex fatalGuard;
	boost::mutex resultGuard;
	int32_t nodeId;
public:
	BasicLogLib();
	~BasicLogLib();
	void info(const char* str,...);
	void debug(const char* str,...);
	void warn(const char* str,...);
	void error(const char* str,...);
	void fatal(const char* str,...);
	void result(const char* str,...);
	void consoleOut(const char* str,...);
	void consoleError(const char* str,...);
	double getCurrentTime();
};

}
#endif /* DEFAULTLOGLIB_H_ */
