/*
 * ContentionManager.h
 *
 *  Created on: Dec 30, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CONTENTIONMANAGER_H_
#define CONTENTIONMANAGER_H_

namespace vt_dstm {
/*
 * For DTL based Algorithm no Contention Manager in required to resolve the priority etc.
 * Contention Manager is used to perform random back-off in case of too many aborts
 */
class ContentionManager {
public:
	ContentionManager();
	virtual ~ContentionManager();

	static void init(void* metaData);
	static void deInit(void* metaData);
};

} /* namespace vt_dstm */

#endif /* CONTENTIONMANAGER_H_ */
