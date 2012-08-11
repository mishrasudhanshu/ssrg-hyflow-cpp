/*
 * Pantheios.h
 *
 *  Created on: Aug 10, 2012
 *      Author: sudhanshu
 */

#ifndef PANTHEIOS_H_
#define PANTHEIOS_H_

#include "AbstractLogLib.h"

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

//TODO: Use Pantheios Logging Library
class Pantheios: public AbstractLogLib{
public:
	void info(const char* str,...) {};
	void debug(const char* str,...) {};
	void warn(const char* str,...){};
	void error(const char* str,...){};
	void fatal(const char* str,...){};
	void result(const char* str,...){};
};

#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* PANTHEIOS_H_ */
