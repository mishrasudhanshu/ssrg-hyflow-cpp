/*
 * abstractLogLib.h
 *
 *  Created on: Aug 9, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTLOGLIB_H_
#define ABSTRACTLOGLIB_H_

namespace vt_dstm
{
class AbstractLogLib {
public:
	virtual ~AbstractLogLib(){};
	virtual void info(char const* str,...)=0;
	virtual void debug(char const* str,...)=0;
	virtual void warn(char const* str,...)=0;
	virtual void error(char const* str,...)=0;
	virtual void fatal(char const* str,...)=0;
	virtual void result(char const* str,...)=0;
};

}
#endif /* ABSTRACTLOGLIB_H_ */
