#pragma once

extern "C" {
#include <lua.h>
}

#include <string>
#include <unordered_map>

class AliasMap {
public:
	AliasMap() = default;
	void load_from_file(const std::string &path);
	std::string resolve(const std::string &alias) const;

private:
	std::unordered_map<std::string, std::string> m_aliases;
};

extern AliasMap g_myengine_aliases;
