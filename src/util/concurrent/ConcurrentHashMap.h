/*
 * ConcurrentHashMap.h
 *
 *  Created on: Aug 29, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CONCURRENTHASHMAP_H_
#define CONCURRENTHASHMAP_H_

#include <map>
#include <boost/thread/shared_mutex.hpp>

namespace vt_dstm {

//template<class Key,class Value>
//class ConcurrentHashMapBase: std::map<Key, Value> {
//	boost::shared_mutex rwMutex;
//public:
//	ConcurrentHashMapBase() {};
//	virtual ~ConcurrentHashMapBase() {};
//
//	void deletePair(Key & k) {
//		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
//		std::map<Key, Value>::erase(k);
//	}
//
//	Value & operator[](Key & k) {
//		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
//		return std::map<Key, Value>::operator [](k);
//
//	}
//
//	ConcurrentHashMapBase & operator = (ConcurrentHashMapBase & chm) {
//		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
//		return std::map<Key, Value>::operator = (chm);
//
//	}
//
//	Value & getValue(Key & k) {
//		boost::lock_guard<boost::shared_mutex> readlock(rwMutex);
//		return std::map<Key, Value>::at(k);
//	}
//
//};

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
		return chm.at(k);
	}

	void updateValue(std::pair<Key, Value> & p) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
		try {
			chm.erase(p.first);
		}catch(...) {
			//Currently just ignore
		}

		chm.insert(p);
	}

	void insertValue(std::pair<Key, Value> & p) {
		boost::upgrade_lock<boost::shared_mutex> writeLock(rwMutex);
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
