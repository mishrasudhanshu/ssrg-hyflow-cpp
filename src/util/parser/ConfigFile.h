/*
 * ConfigFile.h
 *
 *  Created on: Aug 10, 2012
 *      Author: mishras[at]vt.edu
 */

#ifndef CONFIGFILE_H_
#define CONFIGFILE_H_

#include <map>
#include <string>
#include "../Definitions.h"

namespace vt_dstm
{

class ConfigFile {
	static std::map<std::string, std::string> configMap;
	static bool isInitialized;

public:
	static void ConfigFileInit(std::string const& configFile);

	static std::string const& Value(std::string const& entry);
	static std::string const& Value(std::string const& entry,
			std::string const& value);
	static std::string const& Update(std::string const& entry, std::string const& value);
	static void UpdateMap();
	static void test();
};

}
#endif /* CONFIGFILE_H_ */
