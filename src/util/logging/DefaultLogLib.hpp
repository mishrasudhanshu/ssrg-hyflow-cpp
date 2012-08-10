/*
 * DefaultLogLib.h
 *
 *  Created on: Aug 9, 2012
 *      Author: sudhanshu
 */

#ifndef DEFAULTLOGLIB_H_
#define DEFAULTLOGLIB_H_

#include "AbstractLogLib.hpp"
#include <fstream>
using namespace std;

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

class DefaultLogLib:public AbstractLogLib {
	FILE *infoStr;
	FILE *debugStr;
	FILE *warnStr;
	FILE *errorStr;
	FILE *fatalStr;
public:
	DefaultLogLib();
	~DefaultLogLib();
	void info(const char* str,...);
	void debug(const char* str,...) {};
	void warning(const char* str,...){};
	void error(const char* str,...){};
	void fatal(const char* str,...){};
};


#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* DEFAULTLOGLIB_H_ */
