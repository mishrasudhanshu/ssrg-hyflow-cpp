/*
 * DirectoryManager.h
 *
 *  Created on: Aug 27, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef DIRECTORYMANAGER_H_
#define DIRECTORYMANAGER_H_

#include <string>

#include "../HyflowObject.h"

namespace vt_dstm {

class DirectoryManager {
public:
	DirectoryManager();
	virtual ~DirectoryManager();
	static HyflowObject & getObjectLocally(std::string id, bool rw);
	static int getObjectLocation(std::string nodeId);
};

} /* namespace vt_dstm */

#endif /* DIRECTORYMANAGER_H_ */
