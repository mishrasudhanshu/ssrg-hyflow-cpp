/*
 * DefaultLogLib.h
 *
 *  Created on: Aug 9, 2012
 *      Author: sudhanshu
 */

#ifndef DEFAULTLOGLIB_H_
#define DEFAULTLOGLIB_H_

#include <fstream>

#include "AbstractLogLib.h"

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

class BasicLogLib:public AbstractLogLib {
	FILE *infoStr;
	FILE *debugStr;
	FILE *warnStr;
	FILE *errorStr;
	FILE *fatalStr;
	FILE *resultStr;
	int nodeId;
public:
	BasicLogLib();
	~BasicLogLib();
	void info(const char* str,...);
	void debug(const char* str,...);
	void warn(const char* str,...);
	void error(const char* str,...);
	void fatal(const char* str,...);
	void result(const char* str,...);
	double getCurrentTime();
};


#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* DEFAULTLOGLIB_H_ */
