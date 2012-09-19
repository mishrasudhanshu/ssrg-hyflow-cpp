/*
 * ThreadId.cpp
 *
 *  Created on: Sep 19, 2012
 *      Author: mishras[at]vt.edu
 */
#include "ThreadId.h"

namespace vt_dstm{
	boost::thread_specific_ptr<HyInteger> ThreadId::threadId;
}


