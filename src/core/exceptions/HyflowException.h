/*
 * HyflowException.h
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWEXCEPTION_H_
#define HYFLOWEXCEPTION_H_

#include <exception>

namespace vt_dstm {

class HyflowException: public std::exception {
public:
	HyflowException() {};
	virtual ~HyflowException() throw () {};
};

} /* namespace vt_dstm */

#endif /* HYFLOWEXCEPTION_H_ */
