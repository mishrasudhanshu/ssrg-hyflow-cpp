/*
 * abstractLogLib.h
 *
 *  Created on: Aug 9, 2012
 *      Author: sudhanshu
 */

#ifndef ABSTRACTLOGLIB_H_
#define ABSTRACTLOGLIB_H_


#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

class AbstractLogLib {
public:
	virtual ~AbstractLogLib(){};
	virtual void info(char const* str,...)=0;
	virtual void debug(char const* str,...)=0;
	virtual void warning(char const* str,...)=0;
	virtual void error(char const* str,...)=0;
	virtual void fatal(char const* str,...)=0;
};

#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* ABSTRACTLOGLIB_H_ */
