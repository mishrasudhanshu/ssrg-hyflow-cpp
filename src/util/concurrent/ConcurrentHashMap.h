/*
 * ConcurrentHashMap.h
 *
 *  Created on: Aug 29, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CONCURRENTHASHMAP_H_
#define CONCURRENTHASHMAP_H_

#include <map>
#include <iostream>
#include <boost/thread/shared_mutex.hpp>

namespace vt_dstm {

template<class Key,class Value>
class ConcurrentHashMap{
	std::map<Key, Value> chm;
	boost::shared_mutex rwMutex;
	void deleteInternal(Key & k) {
		chm.erase(k);
	}
public:
	ConcurrentHashMap() {};
	virtual ~ConcurrentHashMap() {};

	void deletePair(Key & k) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
		chm.erase(k);
	}

	Value & getValue(Key & k) {
		boost::shared_lock<boost::shared_mutex> readLock(rwMutex);
		return chm.at(k);
	}

	void updateValue(std::pair<Key, Value> & p) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
		try {
			chm.erase(p.first);
		}catch(...) {
			std::cerr<<"Concurrent HashMap: exception erase"<<std::endl;
		}

		chm.insert(p);
	}

	void updatePointerValue(std::pair<Key, Value> & p) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
		try {
//			Value v = chm.at(p.first);
			chm.erase(p.first);
//			delete v;
		}catch (std::out_of_range & e) {
			// Means object was not already there
			// No need to erase it and delete its pointer
			std::cerr<<"Concurrent HashMap: Out of range exception thrown!!"<<std::endl;
		}
		catch(...) {
			std::cerr<<"Concurrent HashMap: exception erase"<<std::endl;
		}
		chm.insert(p);
	}

	void insertValue(std::pair<Key, Value> & p) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
		try {
			chm.erase(p.first);
		}catch(...) {
			std::cerr<<"Concurrent HashMap: exception erase"<<std::endl;
		}
		chm.insert(p);
	}

	void insertValue(Key & k, Value & v) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
		chm[k] = v;
	}

	static void test() {
		ConcurrentHashMap<int, int> prMap;
		int ikey=1;
		int ivalue = 2;
		std::pair<int, int> pi;
		pi.first = ikey;
		pi.second = ivalue;
		prMap.insertValue(pi);

		if (!((prMap.getValue(ikey) == 2)) ) {
			std::cerr<<"ConcurrentHashMap Test failed: float -"<<prMap.getValue(ikey)<<"\n";
		}

		ConcurrentHashMap<std::string, std::string> secMap;
		std::string skey = "Hello";
		std::string svalue = "there";

		std::pair<std::string, std::string> ps;
		ps.first = skey;
		ps.second = svalue;
		secMap.insertValue(ps);
		std::string v = secMap.getValue(skey);
		if (secMap.getValue(skey).compare("there") == 0) {
			std::cout<<"ConcurrentHashMap Test passed\n";
		} else {
			std::cerr<<"ConcurrentHashMap Test failed: string -"<<secMap.getValue(skey)<<"\n";
		}
	}

};

} /* namespace vt_dstm */

#endif /* CONCURRENTHASHMAP_H_ */
