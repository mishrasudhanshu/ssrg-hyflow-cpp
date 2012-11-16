/*
 * objectIdComparator.h
 *
 *  Created on: Sep 4, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef OBJECTIDCOMPARATOR_H_
#define OBJECTIDCOMPARATOR_H_

#include <string>
#include <cstdlib>

namespace vt_dstm {
/*
 * Helper class to sort the object Id string
 */
class ObjectIdComparator {
   int sign;
public:
   ObjectIdComparator() {sign = 1;}
   ObjectIdComparator(int s):sign(s){}

   /**
    * returns id1>id2 if sign>0, else id1<id2
    */
   bool operator()(std::string id1,std::string id2) {
	   int obj1 = atoi(id1.substr(id1.find('-')+1).c_str());
	   int obj2 = atoi(id2.substr(id2.find('-')+1).c_str());
	   if ( obj1==obj2 ) {
		   int node1 = atoi(id1.substr(0, id1.find('-')).c_str());
		   int node2 = atoi(id2.substr(0, id2.find('-')).c_str());
		   return sign*(node1-node2)>0;
	   }
	   return sign*(obj1-obj2)>0;
   }
};

} /* namespace vt_dstm */

#endif /* OBJECTIDCOMPARATOR_H_ */
