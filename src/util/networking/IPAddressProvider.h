/*
 * IPAdressProvider.h
 *
 *  Created on: Sep 26, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef IPADDRESSPROVIDER_H_
#define IPADDRESSPROVIDER_H_

#include <string>

namespace vt_dstm {

class IPAddressProvider {
	static std::string ipvAddress[2];
	static bool init;

	static void InitIPs();
	static void setIps(int domain);
public:
	IPAddressProvider();
	virtual ~IPAddressProvider();

	static std::string getIPv4Address();
	static std::string getIPv6Address();
};

} /* namespace vt_dstm */

#endif /* IPADDRESSPROVIDER_H_ */
