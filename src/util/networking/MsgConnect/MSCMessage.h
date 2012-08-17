/*
 * MSCMessage.h
 *
 *  Created on: Aug 16, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef MSCMESSAGE_H_
#define MSCMESSAGE_H_

#include "../AbstractMessage.h"

namespace vt_dstm
{

class MSCMessage: public AbstractMessage {
public:
	MSCMessage();
	virtual ~MSCMessage();
};

}
#endif /* MSCMESSAGE_H_ */
