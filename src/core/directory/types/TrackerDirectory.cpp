/*
 * TrackerDirectory.cpp
 *
 *  Created on: Aug 28, 2012
 *      Author: mishras[at]vt.edu
 */

#include "TrackerDirectory.h"
#include "../../../util/networking/NetworkManager.h"
#include "../../../util/messages/types/ObjectTrackerMsg.h"
#include "../../../util/messages/types/RegisterObjectMsg.h"
#include "../../../util/logging/Logger.h"

namespace vt_dstm {
ConcurrentHashMap<std::string, int> TrackerDirectory::directory;
ConcurrentHashMap<std::string, HyflowObject*> TrackerDirectory::local;

TrackerDirectory::TrackerDirectory() {}

TrackerDirectory::~TrackerDirectory() {}

int TrackerDirectory::getTracker(std::string objectId) {
	int end = objectId.find('-');
	return atoi((objectId.substr(0,end)).c_str());
}

HyflowObject* TrackerDirectory::locate(std::string id, bool rw, unsigned long long txn){
	HyflowObjectFuture fu(id, rw, txn);
	locateAsync(id, rw, txn, fu);
	return fu.waitOnObject();
}

void TrackerDirectory::locateAsync(std::string id, bool rw, unsigned long long txn, HyflowObjectFuture & fu){
	ObjectTrackerMsg msg(id, rw);
	int trackerNode = getTracker(id);

	HyflowMessage hmsg;
	hmsg.msg_t = MSG_TRK_OBJECT;
	hmsg.isCallback = true;	// isReplied false by default
	hmsg.setMsg(&msg);
	NetworkManager::sendCallbackMessage(trackerNode,hmsg,fu.getMessageFuture());
}

void TrackerDirectory::registerObject(HyflowObject & object, unsigned long long txn) {
	Logger::debug("DIR: Registering a object\n");
	std::string & id = object.getId();
	//Save Object locally
	std::pair<std::string, HyflowObject*> p;
	p.first = object.getId();
	p.second = (HyflowObject*)&object;
	local.insertValue(p);
	//Register the object with tracker
	int owner  = NetworkManager::getNodeId();
	RegisterObjectMsg romsg(id, owner, txn);
	HyflowMessage hmsg;
	hmsg.msg_t = MSG_REGISTER_OBJ;	// Not callback by default
	hmsg.setMsg(&romsg);
	NetworkManager::sendMessage(getTracker(id), hmsg);
}

void TrackerDirectory::registerObjectLocally(std::string objId, int owner, unsigned long long txn) {
	std::pair<std::string, int> p;
	p.first = objId;
	p.second = owner;
	Logger::debug("DIR : Object %s owner %d\n", objId.c_str(), owner);
	directory.insertValue(p);
}

void TrackerDirectory::unregisterObject(HyflowObject & object, unsigned long long txn) {
	RegisterObjectMsg romsg(object.getId(),txn);
	HyflowMessage hmsg;
	hmsg.msg_t = MSG_REGISTER_OBJ;	// Not callback by default
	hmsg.setMsg(&romsg);
	NetworkManager::sendMessage(getTracker(object.getId()), hmsg);
}

// TODO: Proper clean up will require object to be deleted from all local caches
void TrackerDirectory::unregisterObjectLocally(std::string objId, unsigned long long txn) {
	directory.deletePair(objId);
}

HyflowObject & TrackerDirectory::getObjectLocally(std::string id, bool rw){
	return *local.getValue(id);
}


void TrackerDirectory::updateObjectLocally(HyflowObject & obj){
	std::pair<std::string, HyflowObject*> p;
	p.first = obj.getId();
	p.second = &obj;
	local.updateValue(p);
}

int TrackerDirectory::getObjectLocation(std::string id){
	return directory.getValue(id);
}

} /* namespace vt_dstm */
