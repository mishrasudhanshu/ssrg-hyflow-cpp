/*
 * Pantheios.h
 *
 *  Created on: Aug 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef PANTHEIOS_H_
#define PANTHEIOS_H_

#include "../AbstractLogLib.h"

namespace vt_dstm
{

//TODO: Use Pantheios Logging Library
class Pantheios: public AbstractLogLib{
public:
	void info(const char* str,...) {};
	void debug(const char* str,...) {};
	void warn(const char* str,...){};
	void error(const char* str,...){};
	void fatal(const char* str,...){};
	void result(const char* str,...){};
	void consoleOut(const char* str,...){};
	void consoleError(const char* str,...){};
};

}
#endif /* PANTHEIOS_H_ */
