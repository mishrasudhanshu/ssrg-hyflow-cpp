/*
 * AbstractMessage.h
 *
 *  Created on: Aug 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef ABSTRACTMESSAGE_H_
#define ABSTRACTMESSAGE_H_

#ifdef HY_USE_NAMESPACE
namespace vt_dstm
{
#endif


enum MessageType {
	MSG_READ_RQ, MSG_READ_RS, MSG_WRITE_RQ, MSG_WRITE_RS, MSG_COMMIT_RQ, MSG_COMMIT_RS, MSG_COMMIT_FN
};

class AbstractMessage {
public:
	virtual ~AbstractMessage(){};

	MessageType msg_t;
	bool isCallback;
	int fromNode;
	int toNode;
	void *data;
};

#ifdef HY_USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* ABSTRACTMESSAGE_H_ */
