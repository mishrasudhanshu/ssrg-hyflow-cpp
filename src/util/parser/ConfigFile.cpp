/*
 * ConfigFile.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: mishras[at]vt.edu
 */

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "ConfigFile.h"
#include "../Definitions.h"

namespace vt_dstm {
std::map<std::string, std::string> ConfigFile::configMap;
bool ConfigFile::isInitialized = false;

static std::string trim(std::string const& source, char const* delims =
		" \t\r\n") {
	std::string result(source);
	std::string::size_type index = result.find_last_not_of(delims);
	if (index != std::string::npos)
		result.erase(++index);

	index = result.find_first_not_of(delims);
	if (index != std::string::npos)
		result.erase(0, index);
	else
		result.erase();
	return result;
}

void ConfigFile::ConfigFileInit(std::string const& configFile) {
	if (!isInitialized) {
		std::ifstream file(configFile.c_str());

		std::string line;
		std::string name;
		std::string value;
		std::string inSection;
		int posEqual;
		while (std::getline(file, line)) {

			if (!line.length())
				continue;

			if (line[0] == '#')
				continue;
			if (line[0] == ';')
				continue;

			posEqual = line.find('=');
			name = trim(line.substr(0, posEqual));
			value = trim(line.substr(posEqual + 1));

			configMap[name] = value;
		}
		file.close();
		isInitialized = true;
	}
}

std::string const& ConfigFile::Value(std::string const& entry) {

	std::map<std::string, std::string>::const_iterator ci = configMap.find(
			entry);

	if (ci == configMap.end())
		throw entry.c_str();

	return ci->second;
}

std::string const& ConfigFile::Value(std::string const& entry,
		std::string const& value) {
	try {
		return Value(entry);
	} catch (const char *) {
		return configMap.insert(std::make_pair(entry, value)).first->second;
	}
}

std::string const& ConfigFile::Update(std::string const& entry,
		std::string const& value) {
	try {
		Value(entry);
		configMap.erase(entry);
		configMap[entry] = value;
		return value;
	} catch (const char *x) {
		std::cerr << "Specified key " << x << " not defined in default.conf"
				<< std::endl;
	}
	return value;
}

void ConfigFile::UpdateMap() {
	char* value = NULL;

	value = getenv(DIRECTORY_MANAGER);
	if (value)
		Update(DIRECTORY_MANAGER, value);

	value = getenv(NETWORK);
	if (value)
		Update(NETWORK, value);

	value = getenv(ZERO_MQ_TFR);
	if (value)
		Update(ZERO_MQ_TFR, value);

	value = getenv(ZERO_MQ_WFR);
	if (value)
		Update(ZERO_MQ_WFR, value);

	value = getenv(CONTEXT);
	if (value)
		Update(CONTEXT, value);

	value = getenv(LOGGER);
	if (value)
		Update(LOGGER, value);

	value = getenv(CONTENTION_POLICY);
	if (value)
		Update(CONTENTION_POLICY, value);

	value = getenv(REMOTE_CALLER);
	if (value)
		Update(REMOTE_CALLER, value);

	value = getenv(VERBOSE);
	if (value)
		Update(VERBOSE, value);

	value = getenv(HY_DEBUG);
	if (value)
		Update(HY_DEBUG, value);

	value = getenv(SANITY);
	if (value)
		Update(SANITY, value);

	value = getenv(INSTRUMENT);
	if (value)
		Update(INSTRUMENT, value);

	value = getenv(CHECKPOINT);
	if (value)
		Update(CHECKPOINT, value);

	value = getenv(LINK_DELAY);
	if (value)
		Update(LINK_DELAY, value);

	value = getenv(CALL_COST);
	if (value)
		Update(CALL_COST, value);

	value = getenv(PARENT_IP);
	if (value)
		Update(PARENT_IP, value);

	value = getenv(MY_IP);
	if (value)
		Update(MY_IP, value);

	value = getenv(BASE_PORT);
	if (value)
		Update(BASE_PORT, value);

	value = getenv(TERMINATE_IDLE);
	if (value)
		Update(TERMINATE_IDLE, value);

	value = getenv(MACHINES);
	if (value)
		Update(MACHINES, value);

	value = getenv(UNIT_TEST);
	if (value)
		Update(UNIT_TEST, value);

	value = getenv(BENCHMARK);
	if (value)
		Update(BENCHMARK, value);

	value = getenv(NODES);
	if (value)
		Update(NODES, value);

	value = getenv(NODE_ID);
	if (value)
		Update(NODE_ID, value);

	value = getenv(OBJECTS);
	if (value)
		Update(OBJECTS, value);

	value = getenv(NESTING);
	if (value)
		Update(NESTING, value);

	value = getenv(CALLS);
	if (value)
		Update(CALLS, value);

	value = getenv(READS);
	if (value)
		Update(READS, value);

	value = getenv(THREADS);
	if (value)
		Update(THREADS, value);

	value = getenv(TRANSACTIONS);
	if (value)
		Update(TRANSACTIONS, value);

	value = getenv(TRANSACTIONS_LENGTH);
	if (value)
		Update(TRANSACTIONS_LENGTH, value);

	value = getenv(TIMEOUT);
	if (value)
		Update(TIMEOUT, value);

	value = getenv(NESTING_MODEL);
	if (value)
		Update(NESTING_MODEL, value);
}

void ConfigFile::test() {
	std::cout << "\n---Testing ConfigFile---\n" << std::endl;
	ConfigFileInit("default.conf");

	std::string directoryManager;
	std::string comm_manager;
	std::string context;
	std::string cp;
	std::string remoteCaller;
	std::string v;
	std::string d;
	std::string s;
	std::string i;
	std::string checkPoint;
	std::string l;
	std::string cc;
	std::string p;
	std::string m;
	std::string b;
	std::string t;
	std::string mc;
	std::string ut;

	std::string n;
	std::string o;
	std::string ns;
	std::string c;
	std::string r;
	std::string th;
	std::string tr;
	std::string trl;
	std::string to;
	std::string nM;

	try {
		directoryManager = Value(DIRECTORY_MANAGER);
		comm_manager = Value(NETWORK);
		context = Value(CONTEXT);
		cp = Value(CONTENTION_POLICY);
		remoteCaller = Value(REMOTE_CALLER);
		v = Value(VERBOSE);
		d = Value(HY_DEBUG);
		s = Value(SANITY);
		i = Value(INSTRUMENT);
		checkPoint = Value(CHECKPOINT);
		l = Value(LOGGER);
		cc = Value(CALL_COST);
		p = Value(PARENT_IP);
		m = Value(MY_IP);
		b = Value(BASE_PORT);
		t = Value(TERMINATE_IDLE);
		mc = Value(MACHINES);
		ut = Value(UNIT_TEST);
		n = Value(NODES);
		o = Value(OBJECTS);
		ns = Value(NESTING);
		c = Value(CALLS);
		r = Value(READS);
		th = Value(THREADS);
		tr = Value(TRANSACTIONS);
		trl = Value(TRANSACTIONS_LENGTH);
		to = Value(TIMEOUT);
		nM = Value(NESTING_MODEL);
	} catch (const char *x) {
		std::cerr << "Can not find " << x << "\n\n!!!FAILED!!!\n" << std::endl;
	}
	std::cout << DIRECTORY_MANAGER <<": " << directoryManager << std::endl;
	std::cout << NETWORK <<": " << comm_manager << std::endl;
	std::cout << CONTEXT <<": " << context << std::endl;
	std::cout << CONTENTION_POLICY <<": " << cp << std::endl;
	std::cout << REMOTE_CALLER <<": " << remoteCaller << std::endl;
	std::cout << VERBOSE <<": " << v << std::endl;
	std::cout << HY_DEBUG <<": " << d << std::endl;
	std::cout << SANITY <<": " << s << std::endl;
	std::cout << INSTRUMENT <<": " << i << std::endl;
	std::cout << CHECKPOINT <<": " << checkPoint << std::endl;
	std::cout << LOGGER <<": " << l << std::endl;
	std::cout << CALL_COST <<": " << cc << std::endl;
	std::cout << PARENT_IP <<": " << p << std::endl;
	std::cout << MY_IP <<": " << m << std::endl;
	std::cout << BASE_PORT <<": " << b << std::endl;
	std::cout << TERMINATE_IDLE <<": " << t << std::endl;
	std::cout << MACHINES <<": " << mc << std::endl;
	std::cout << UNIT_TEST <<": " << ut << std::endl;
	std::cout << NODES <<": " << n << std::endl;
	std::cout << OBJECTS <<": " << o << std::endl;
	std::cout << NESTING <<": " << ns << std::endl;
	std::cout << CALLS <<": " << c << std::endl;
	std::cout << READS <<": " << r << std::endl;
	std::cout << THREADS <<": " << th << std::endl;
	std::cout << TRANSACTIONS <<": " << tr << std::endl;
	std::cout << TRANSACTIONS_LENGTH <<": " << trl << std::endl;
	std::cout << TIMEOUT <<": " << to << std::endl;
	std::cout << NESTING_MODEL <<": " << nM << std::endl;

}

}
