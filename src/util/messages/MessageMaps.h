/*
 * MessageMaps.h
 *
 *  Created on: Sep 18, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef MESSAGEMAPS_H_
#define MESSAGEMAPS_H_

#include "tbb/concurrent_hash_map.h"
#include "../concurrent/ConcurrentHashMap.h"
#include "../messages/HyflowMessageFuture.h"

namespace vt_dstm {
class MessageMaps {
	tbb::concurrent_hash_map<HyMessageType, void (*)(HyflowMessage &)>* handlerMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* trackerCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* objCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* syncCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* lockCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* readValidCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* registerCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* dummyCallbackMap;
	tbb::concurrent_hash_map<std::string, HyflowMessageFuture*>* AbstractLockCallbackMap;
	static MessageMaps* instance;
	MessageMaps();
public:
	virtual ~MessageMaps();
	static void MessageMapsInit();

	static void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &));
	static void(*getMessageHandler(HyMessageType type))(HyflowMessage & hmsg);

	static void registerMessageFuture(const std::string & m_id, HyMessageType t, HyflowMessageFuture & fu);
	static HyflowMessageFuture* getMessageFuture(const std::string & m_id, HyMessageType t);
	static void removeMessageFuture(const std::string & m_id, HyMessageType t);
};
} /* namespace vt_dstm */


#endif /* MESSAGEMAPS_H_ */
