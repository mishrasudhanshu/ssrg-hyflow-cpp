/*
 * ClusterManager.h
 *
 *  Created on: Aug 10, 2012
 *      Author: sudhanshu
 */

#ifndef CLUSTERMANAGER_H_
#define CLUSTERMANAGER_H_

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

class ClusterManager {
public:
	ClusterManager();
	virtual ~ClusterManager();

	static int getClusterId();
};

#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* CLUSTERMANAGER_H_ */
