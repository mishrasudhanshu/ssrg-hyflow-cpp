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
 * Supports any Node-Item-objectNo type Id
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
	   if (id1.substr(0, id1.find('-')).compare("HEAD") == 0) {
		   if (id1.find('-') != std::string::npos) {	//HEAD-***
			   if ((id2.substr(0, id2.find('-')).compare("HEAD") == 0)
					   && (id2.find('-') != std::string::npos)) {
				   // Both are head return based on item number
				   int item1 = atoi(id1.substr(id1.find('-')+1).c_str());
				   int item2 = atoi(id2.substr(id2.find('-')+1).c_str());
				   return sign*(item1-item2)>0;
			   }
			   return false;
		   }
		   return false;
	   }else if(id2.substr(0, id2.find('-')).compare("HEAD") == 0) {
		   return true;
	   }
	   int obj1 = atoi(id1.substr(id1.find_last_of('-')+1).c_str());
	   int obj2 = atoi(id2.substr(id2.find_last_of('-')+1).c_str());
	   if ( obj1==obj2 ) {
		   int node1 = atoi(id1.substr(0, id1.find('-')).c_str());
		   int node2 = atoi(id2.substr(0, id2.find('-')).c_str());
		   if (node1 == node2) {
			   int item1 = atoi(id1.substr(id1.find('-')+1, id1.find_last_of('-')).c_str());
			   int item2 = atoi(id2.substr(id2.find('-')+1, id2.find_last_of('-')).c_str());
			   return sign*(item1-item2)>0;
		   }
		   return sign*(node1-node2)>0;
	   }
	   return ((sign*(obj1-obj2)) > 0);
   }
};

} /* namespace vt_dstm */

#endif /* OBJECTIDCOMPARATOR_H_ */
