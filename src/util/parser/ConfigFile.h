/*
 * ConfigFile.h
 *
 *  Created on: Aug 10, 2012
 *      Author: sudhanshu
 */

#ifndef CONFIGFILE_H_
#define CONFIGFILE_H_

#include <map>
#include <string>

#ifdef USE_NAMESPACE
namespace VT_DSTM
{
#endif

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

#ifdef USE_NAMESPACE
}
#endif /* NAME_SPACE VT_DSTM */
#endif /* CONFIGFILE_H_ */
