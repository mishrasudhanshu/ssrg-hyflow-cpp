/*
 * HashBucket.h
 *
 *  Created on: Nov 17, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef HASHBUCKET_H_
#define HASHBUCKET_H_

namespace vt_dstm {

class HashBucket {
	std::vector<std::pair<int, double>> bucket;
public:
	HashBucket();
	virtual ~HashBucket();
};

} /* namespace vt_dstm */

#endif /* HASHBUCKET_H_ */
