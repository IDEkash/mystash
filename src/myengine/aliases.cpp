#include "aliases.h"
#include "log.h"
#include "filesys.h"
#include <fstream>
#include <sstream>

AliasMap g_myengine_aliases;

void AliasMap::load_from_file(const std::string &path)
{
	std::ifstream infile(path);
	if (!infile.is_open()) {
		warningstream << "myengine: Could not open alias map file: " << path << std::endl;
		return;
	}

	std::string line;
	while (std::getline(infile, line)) {
		if (line.empty() || line[0] == '#')
			continue;

		size_t eq_pos = line.find('=');
		if (eq_pos == std::string::npos)
			continue;

		std::string key = line.substr(0, eq_pos);
		std::string val = line.substr(eq_pos + 1);

		// Trim whitespace
		key.erase(0, key.find_first_not_of(" \t"));
		key.erase(key.find_last_not_of(" \t") + 1);
		val.erase(0, val.find_first_not_of(" \t"));
		val.erase(val.find_last_not_of(" \t") + 1);

		if (!key.empty() && !val.empty()) {
			m_aliases[key] = val;
		}
	}
	infostream << "myengine: Loaded " << m_aliases.size() << " aliases from " << path << std::endl;
}

std::string AliasMap::resolve(const std::string &alias) const
{
	auto it = m_aliases.find(alias);
	if (it != m_aliases.end()) {
		return it->second;
	}
	return alias;
}
