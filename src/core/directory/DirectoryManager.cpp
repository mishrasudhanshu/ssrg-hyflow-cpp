/*
 * DirectoryManager.cpp
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#include "DirectoryManager.h"

namespace vt_dstm {

DirectoryManager::DirectoryManager() {
	// TODO Auto-generated constructor stub

}

DirectoryManager::~DirectoryManager() {
	// TODO Auto-generated destructor stub
}

HyflowObject & DirectoryManager::getObjectLocally(std::string id, bool rw) {
	HyflowObject dummy;
	return dummy;
}

int DirectoryManager::getObjectLocation(std::string nodeId) {
	return 0;
}

} /* namespace vt_dstm */
