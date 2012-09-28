/*
 * IPAddressProvider.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: mishras[at]vt.edu
 */

#include <stdio.h>
#include <stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "IPAddressProvider.h"
#include "../logging/Logger.h"
#include "NetworkManager.h"

namespace vt_dstm {

bool IPAddressProvider::init =false;
std::string IPAddressProvider::ipvAddress[2];

IPAddressProvider::IPAddressProvider() {
}

IPAddressProvider::~IPAddressProvider() {
}

void IPAddressProvider::InitIPs() {
	//FIXME: Add the IPv6 support
	int domains[] = {AF_INET/*,AF_INET6*/};

	for (int itr=0 ; itr < 1; itr++) {
		int domain = domains[itr];
		int s;
		struct ifconf ifconf;
		struct ifreq ifr[50];
		int ifs;
		int i;

		s = socket(domain, SOCK_STREAM, 0);
		if (s < 0) {
			perror("socket");
			return;
		}

		ifconf.ifc_buf = (char *) ifr;
		ifconf.ifc_len = sizeof ifr;

		if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
			perror("ioctl");
			return;
		}

		ifs = ifconf.ifc_len / sizeof(ifr[0]);
//		printf("interfaces = %d:\n", ifs);
		// Read only last interface value
//		for (i = 0; i < ifs; i++)
		i = ifs -1;
		{
			char ip[INET_ADDRSTRLEN];
			struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

			if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) {
				perror("inet_ntop");
				return;
			}

//			printf("%s - %s\n", ifr[i].ifr_name, ip);
			ipvAddress[itr] = ip;
		}
		close(s);
	}
	init = true;
}

std::string IPAddressProvider::getIPv4Address() {
	if (NetworkManager::islocalMachine()) {
		return "127.0.0.1";
	}

	if (!init) {
		InitIPs();
		if (!init) {
			throw "Failed to collect IPs for this machine\n";
		}
	}
	LOG_DEBUG("Node IPv4 Address : %s\n", ipvAddress[0].c_str());
	return ipvAddress[0];
}

std::string IPAddressProvider::getIPv6Address() {
	if (NetworkManager::islocalMachine()) {
		return "::1";
	}

	if (!init) {
		InitIPs();
		if (!init) {
			Logger::fatal("Failed to collect IPs for this machine\n");
			throw "Failed to collect IPs for this machine\n";
		}
	}
	return ipvAddress[1];
}

} /* namespace vt_dstm */
