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

namespace vt_dstm {
std::map<std::string, int> TrackerDirectory::directory;
std::map<std::string, HyflowObject*> TrackerDirectory::local;

TrackerDirectory::TrackerDirectory() {}

TrackerDirectory::~TrackerDirectory() {}

int TrackerDirectory::getTracker(std::string objectId) {
	int end = objectId.find('-');
	return atoi((objectId.substr(0,end)).c_str());
}

HyflowObject & TrackerDirectory::locate(std::string id, bool rw, unsigned long long txn){
	HyflowObjectFuture fu(id, rw, txn);
	locateAsync(id, rw, txn, fu);
	return fu.waitOnObject();
}

void TrackerDirectory::locateAsync(std::string id, bool rw, unsigned long long txn, HyflowObjectFuture & fu){
	ObjectTrackerMsg msg(id, rw);
	int trackerNode = getTracker(id);
	HyflowMessage hmsg;
	hmsg.setMsg(&msg);
	NetworkManager::sendCallbackMessage(trackerNode,hmsg,fu.getMessageFuture());
}

void TrackerDirectory::registerObject(HyflowObject & object, unsigned long long txn) {
	RegisterObjectMsg romsg(&object,txn);
	HyflowMessage hmsg;
	hmsg.setMsg(&romsg);
	NetworkManager::sendMessage(getTracker(object.getId()), hmsg);
}

void TrackerDirectory::registerObjectLocally(HyflowObject & object, unsigned long long txn) {
	directory[object.getId()] = NetworkManager::getNodeId();
	HyflowObject *obj = NULL;
	object.getClone(obj);
	local[object.getId()] = obj;
}

void TrackerDirectory::unregisterObject(HyflowObject & object, unsigned long long txn) {
	RegisterObjectMsg romsg(object.getId(),txn);
	HyflowMessage hmsg;
	hmsg.setMsg(&romsg);
	NetworkManager::sendMessage(getTracker(object.getId()), hmsg);
}

void TrackerDirectory::unregisterObjectLocally(std::string objId, unsigned long long txn) {
	directory.erase(objId);
	local.erase(objId);
}

HyflowObject & TrackerDirectory::getObjectLocally(std::string id, bool rw){
	return *local[id];
}


void TrackerDirectory::updateObjectLocally(HyflowObject & obj){
	local[obj.getId()] = &obj;
}

int TrackerDirectory::getObjectLocation(std::string id){
	return directory[id];
}

} /* namespace vt_dstm */
