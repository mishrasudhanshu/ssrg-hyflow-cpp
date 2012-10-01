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
	static tbb::concurrent_hash_map<HyMessageType, void (*)(HyflowMessage &)> handlerMap;
	static tbb::concurrent_hash_map<std::string, HyflowMessageFuture*> trackerCallbackMap;
	static tbb::concurrent_hash_map<std::string, HyflowMessageFuture*> objCallbackMap;
	static tbb::concurrent_hash_map<std::string, HyflowMessageFuture*> syncCallbackMap;
	static tbb::concurrent_hash_map<std::string, HyflowMessageFuture*> lockCallbackMap;
	static tbb::concurrent_hash_map<std::string, HyflowMessageFuture*> readValidCallbackMap;
	static tbb::concurrent_hash_map<std::string, HyflowMessageFuture*> registerCallbackMap;
public:
	MessageMaps();
	virtual ~MessageMaps();

	static void registerHandler(HyMessageType msg_t, void (*handlerFunc)(HyflowMessage &));
	static void(*getMessageHandler(HyMessageType type))(HyflowMessage & hmsg);

	static void registerMessageFuture(const std::string & m_id, HyMessageType t, HyflowMessageFuture & fu);
	static HyflowMessageFuture* getMessageFuture(const std::string & m_id, HyMessageType t);
	static void removeMessageFuture(const std::string & m_id, HyMessageType t);
};
} /* namespace vt_dstm */


#endif /* MESSAGEMAPS_H_ */
