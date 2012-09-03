/*
 * dtl2Context.h
 *
 *  Created on: Sep 1, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DTL2CONTEXT_H_
#define DTL2CONTEXT_H_

#include "../HyflowContext.h"

namespace vt_dstm {

class DTL2Context: public vt_dstm::HyflowContext {
public:
	DTL2Context();
	virtual ~DTL2Context();

	void commit();
	unsigned long long getTransactionId();
};

} /* namespace vt_dstm */

#endif /* DTL2CONTEXT_H_ */
