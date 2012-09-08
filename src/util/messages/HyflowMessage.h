/*
 * HyflowMessage.h
 *
 * A wrapper class which is used to send and receive messages
 *
 *  Created on: Aug 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWMESSAGE_H_
#define HYFLOWMESSAGE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include "BaseMessage.h"
#include "../../core/context/ContextManager.h"

namespace vt_dstm
{

enum HyMessageType {
	MSG_GRP_SYNC, /*Group Joining Request*/
	MSG_TRK_OBJECT, /*Object location tracker*/
	MSG_ACCESS_OBJECT, /*Object Read-Write Request/Response*/
	MSG_REGISTER_OBJ, /*Register or Unregister object in cluster*/
	MSG_LOCK_ACCESS,  /*Request object lock unlock*/
	MSG_READ_VALIDATE, /*Validate a object version*/
	MSG_COMMIT_RQ, /*Object Commit Request*/
	MSG_COMMIT_RS, /*Object Commit Response*/
	MSG_COMMIT_FN /*Object Commit Finish*/
};

class HyflowMessage {
	BaseMessage *msg;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
    	registerMessageTypes(ar);
    	ar & msg;
        ar & size;
        ar & msg_t;
        ar & msg_id;
        ar & isCallback;
        ar & fromNode;
        ar & toNode;
        ar & fromNodeClock;
    }
public:
	int size;
	HyMessageType msg_t;
	unsigned long long msg_id;
	int fromNodeClock;

	//LESSON: Boost serialisation requires the bool to initialised
	HyflowMessage(){
		isCallback = false; isReplied = false; msg_id = 0;
		fromNodeClock = ContextManager::getClock();
	}

	virtual ~HyflowMessage(){}

	bool isCallback;
	int fromNode;
	bool isReplied;
	int toNode;

	void init(HyMessageType msg_t, bool isCallback);
	int getSize() {return size;}
	void setSize() {};

	void setMsg(BaseMessage *m){msg=m;}
	BaseMessage* getMsg(){return msg;}

	void setMsgId(unsigned long id){msg_id = id;}
	unsigned long getMsgId(){return msg_id;}

	/**
	 * Called by default handler on message receive
	 */
	void syncClocks();

    template<class Archive>
	static void registerMessageTypes(Archive & ar);

    static void registerMessageHandlers();
    static void test();

};

//LESSON: Useful in case of some other type of compiler
BOOST_SERIALIZATION_ASSUME_ABSTRACT(HyflowMessage)
} /* NAME_SPACE VT_DSTM */

#endif /* HYFLOWMESSAGE_H_ */
