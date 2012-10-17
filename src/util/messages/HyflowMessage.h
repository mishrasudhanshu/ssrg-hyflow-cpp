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

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include "BaseMessage.h"

namespace vt_dstm
{

enum HyMessageType {
	MSG_TYPE_INVALID=0, /*For Dummay Message */
	MSG_GRP_SYNC, /*Group Joining Request*/
	MSG_TRK_OBJECT, /*Object location tracker*/
	MSG_ACCESS_OBJECT, /*Object Read-Write Request/Response*/
	MSG_REGISTER_OBJ, /*Register or Unregister object in cluster*/
	MSG_LOCK_ACCESS,  /*Request object lock unlock*/
	MSG_READ_VALIDATE, /*Validate a object version*/
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
        ar & isCallbackSupported;
        ar & fromNode;
        ar & toNode;
        ar & fromNodeClock;
        ar & forObjectId;
    }
    std::string forObjectId;
public:
	int size;
	HyMessageType msg_t;
	std::string msg_id;
	int fromNodeClock;

	//LESSON: Boost serialisation requires the bool to initialised
	HyflowMessage();
	HyflowMessage(const std::string & forObjectId);

	virtual ~HyflowMessage(){}

	/*
	 * To specify whether networking library used supports the callback or
	 * we manually require to send back the reply.
	 */
	bool isCallbackSupported;
	/*
	 * To specify whether send message will require a reply or not. Used to
	 * identify the one way message.
	 */
	bool isCallback;
	int fromNode;
	int toNode;
	std::string getForObjectId() const;
	void setForObjectId(std::string forObjectId);

	void init(HyMessageType msg_t, bool isCallback);
	int getSize() {return size;}
	void setSize() {};

	void setMsg(BaseMessage *m){msg=m;}
	BaseMessage* getMsg(){return msg;}

	void setMsgId(const std::string & id){msg_id = id;}
	const std::string & getMsgId(){return msg_id;}

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
