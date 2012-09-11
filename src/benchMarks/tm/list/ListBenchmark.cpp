/*
 * ListBenchmark.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: mishras[at]vt.edu
 */

#include <cstdlib>
#include "ListBenchmark.h"

namespace vt_dstm {

ListBenchmark::ListBenchmark() {}

ListBenchmark::~ListBenchmark() {}

int ListBenchmark::getOperandsCount()	{
	return 1;
}

void ListBenchmark::readOperation(std::string ids[], int size){
	if (rand()%2 == 1 ) {
		ListNode::sumNodes();
	}else {
		ListNode::findNode(10);
	}
}

void ListBenchmark::writeOperation(std::string ids[], int size){
	if (rand()%2 == 1 ) {
		ListNode::addNode(10);
	}else {
		ListNode::deleteNode(10);
	}
}

void ListBenchmark::checkSanity(){

}

std::string* ListBenchmark::createLocalObjects(int objCount) {
	return NULL;
}

} /* namespace vt_dstm */
