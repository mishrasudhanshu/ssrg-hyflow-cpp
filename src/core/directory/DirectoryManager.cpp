/*
 * DirectoryManager.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstddef>
#include "DirectoryManager.h"
#include "types/TrackerDirectory.h"
#include "../../util/Definitions.h"
#include "../../util/parser/ConfigFile.h"

namespace vt_dstm {
HyflowDirectory* DirectoryManager::directory = NULL;

void DirectoryManager::DirectoryManagerInit(){
	if (strcmp(ConfigFile::Value(DIRECTORY_MANAGER).c_str(), TRACKER_DIRECTORY) == 0) {
		directory = new TrackerDirectory();
	}
}

HyflowObject & DirectoryManager::locate(std::string id, bool rw, unsigned long long txn){
	return directory->locate(id, rw, txn);
}

void DirectoryManager::locateAsync(std::string id, bool rw, unsigned long long txn, HyflowObjectFuture & fu){
	return directory->locateAsync(id, rw, txn, fu);
}

void DirectoryManager::registerObject(HyflowObject & object, unsigned long long txn){
	directory->registerObject(object,txn);
}

void DirectoryManager::registerObjectLocally(HyflowObject & object, unsigned long long txn){
	directory->registerObjectLocally(object,txn);
}

void DirectoryManager::unregisterObject(HyflowObject & object, unsigned long long txn){
	directory->unregisterObject(object,txn);
}

void DirectoryManager::unregisterObjectLocally(std::string objId, unsigned long long txn){
	directory->unregisterObjectLocally(objId,txn);
}

HyflowObject & DirectoryManager::getObjectLocally(std::string id, bool rw) {
	return directory->getObjectLocally(id, rw);
}

/**
 * Update local object
 */
void DirectoryManager::updateObjectLocally(HyflowObject & obj) {
	directory->updateObjectLocally(obj);
}

int DirectoryManager::getObjectLocation(std::string objId){
	return directory->getObjectLocation(objId);
}

} /* namespace vt_dstm */
