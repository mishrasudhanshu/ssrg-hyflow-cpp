/*
 * AbstractMessage.h
 *
 *  Created on: Aug 14, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HYFLOWMESSAGE_H_
#define HYFLOWMESSAGE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace vt_dstm
{

enum HyMessageType {
	MSG_GRP_RQ, /*Group Joining Request*/
	MSG_GRP_RS, /*Group Joining Response*/
	MSG_CLS_CRT, /*Group Cluster Create*/
	MSG_CLS_CPL, /*Group Cluster Completed*/
	MSG_OBJECT_RQ, /*Object Read-Write Request*/
	MSG_OBJECT_RS, /*Object Read-Write Response*/
	MSG_COMMIT_RQ, /*Object Commit Request*/
	MSG_COMMIT_RS, /*Object Commit Response*/
	MSG_COMMIT_FN /*Object Commit Finish*/
};

class HyflowMessage {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & size;
        ar & msg_t;
        ar & isCallback;
        ar & fromNode;
        ar & isReplied;
        ar & toNode;
    }
public:
	int size;

	//LESSON: Boost serialisation requires the bool to initialised
	HyflowMessage(){isCallback = false; isReplied = false;}
	virtual ~HyflowMessage(){}

	HyMessageType msg_t;
	bool isCallback;
	int fromNode;
	bool isReplied;
	int toNode;

	int getSize() {return size;}
	void setSize() {};

};

//LESSON: Useful in case of some other type of compiler
BOOST_SERIALIZATION_ASSUME_ABSTRACT(HyflowMessage)
} /* NAME_SPACE VT_DSTM */

#endif /* HYFLOWMESSAGE_H_ */
