// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include <cctype>
#include <fstream>
#include <json/json.h>
#include <algorithm>
#include "content/mods.h"
#include "database/database.h"
#include "filesys.h"
#include "log.h"
#include "settings.h"
#include "script/common/c_internal.h"
#include "exceptions.h"

void ModSpec::checkAndLog() const
{
	if (!string_allowed(name, MODNAME_ALLOWED_CHARS)) {
		throw ModError("Error loading mod \"" + name +
			"\": Mod name does not follow naming conventions: "
				"Only characters [a-z0-9_] are allowed.");
	}

	// Log deprecation messages
	auto handling_mode = get_deprecated_handling_mode();
	if (!deprecation_msgs.empty() && handling_mode != DeprecatedHandlingMode::Ignore) {
		std::ostringstream os;
		os << "Mod " << name << " at " << path << ":" << std::endl;
		for (auto msg : deprecation_msgs)
			os << "\t" << msg << std::endl;

		if (handling_mode == DeprecatedHandlingMode::Error)
			throw ModError(os.str());
		else
			warningstream << os.str();
	}
}

bool parseDependsString(std::string &dep, std::unordered_set<char> &symbols)
{
	dep = trim(dep);
	symbols.clear();
	size_t pos = dep.size();
	while (pos > 0 &&
			!string_allowed(dep.substr(pos - 1, 1), MODNAME_ALLOWED_CHARS)) {
		// last character is a symbol, not part of the modname
		symbols.insert(dep[pos - 1]);
		--pos;
	}
	dep = trim(dep.substr(0, pos));
	return !dep.empty();
}

bool parseModContents(ModSpec &spec)
{
	// NOTE: this function works in mutual recursion with getModsInPath

	spec.depends.clear();
	spec.optdepends.clear();
	spec.is_modpack = false;
	spec.modpack_content.clear();

	std::string conf_filename;

	// Handle modpacks (defined by containing modpack.txt) or external.conf (External Logic Packages)
	if (fs::IsFile(spec.path + DIR_DELIM + "external.conf")) {
		conf_filename = "external.conf";
	} else if (fs::IsFile(spec.path + DIR_DELIM + "modpack.conf")) {
		spec.is_modpack = true;
		conf_filename = "modpack.conf";
	} else if (fs::IsFile(spec.path + DIR_DELIM + "modpack.txt")) {
		spec.is_modpack = true;
	} else if (fs::IsFile(spec.path + DIR_DELIM + "init.lua")) {
		conf_filename = "mod.conf";
	} else if (fs::PathExists(spec.path + DIR_DELIM + "ServerSideService") ||
			fs::PathExists(spec.path + DIR_DELIM + "ClientSideService") ||
			fs::PathExists(spec.path + DIR_DELIM + "Workspace")) {
		// Valid External Logic Package without a config file
	} else {
		return false;
	}

	if (spec.is_modpack)
		spec.modpack_content = getModsInPath(spec.path, spec.virtual_path, spec.modpack_depth + 1);

	Settings info;
	if (!conf_filename.empty())
		info.readConfigFile((spec.path + DIR_DELIM + conf_filename).c_str());

	if (info.exists("ID")) {
		spec.name = info.get("ID");
		spec.is_name_explicit = true;
	} else if (info.exists("id")) {
		spec.name = info.get("id");
		spec.is_name_explicit = true;
	} else if (info.exists("name")) {
		spec.name = info.get("name");
		spec.is_name_explicit = true;
	} else if (info.exists("Name")) {
		spec.name = info.get("Name");
		spec.is_name_explicit = true;
	} else if (!spec.is_modpack) {
		spec.deprecation_msgs.push_back("Mods not having a mod.conf or external.conf file with the name is deprecated.");
	}

	if (info.exists("description"))
		spec.desc = info.get("description");
	else if (info.exists("Description"))
		spec.desc = info.get("Description");
	else if (fs::ReadFile(spec.path + DIR_DELIM + "description.txt", spec.desc))
		spec.deprecation_msgs.push_back("description.txt is deprecated, please use mod[pack].conf or external.conf instead.");

	if (info.exists("author"))
		spec.author = info.get("author");
	else if (info.exists("Author"))
		spec.author = info.get("Author");

	if (info.exists("release"))
		spec.release = info.getS32("release");
	else if (info.exists("Release"))
		spec.release = info.getS32("Release");

	// The subsequent fields are not available for modpacks
	if (spec.is_modpack)
		return true;

	// Parse Dependencies block from external.conf if present
	if (conf_filename == "external.conf") {
		std::string file_content;
		if (fs::ReadFile(spec.path + DIR_DELIM + "external.conf", file_content)) {
			size_t dep_pos = file_content.find("Dependencies");
			if (dep_pos != std::string::npos) {
				size_t start_brace = file_content.find('{', dep_pos);
				size_t end_brace = file_content.find('}', dep_pos);
				if (start_brace != std::string::npos && end_brace != std::string::npos && end_brace > start_brace) {
					std::string block = file_content.substr(start_brace + 1, end_brace - start_brace - 1);
					std::vector<std::string> lines = str_split(block, '\n');
					for (auto &line : lines) {
						line = trim(line);
						if (line.empty()) continue;
						size_t name_end = line.find_first_of(" \t><=");
						std::string dep_name = (name_end == std::string::npos) ? line : line.substr(0, name_end);
						dep_name = trim(dep_name);
						if (!dep_name.empty()) {
							spec.depends.insert(dep_name);
						}
					}
				}
			}
		}
	}

	// Attempt to load dependencies from mod.conf / external.conf standard key
	bool mod_conf_has_depends = false;
	if (info.exists("depends") || info.exists("Depends")) {
		mod_conf_has_depends = true;
		std::string dep = info.exists("depends") ? info.get("depends") : info.get("Depends");
		dep.erase(std::remove_if(dep.begin(), dep.end(),
				static_cast<int (*)(int)>(&std::isspace)), dep.end());
		for (const auto &dependency : str_split(dep, ',')) {
			spec.depends.insert(dependency);
		}
	}

	if (info.exists("optional_depends") || info.exists("Optional_depends")) {
		mod_conf_has_depends = true;
		std::string dep = info.exists("optional_depends") ? info.get("optional_depends") : info.get("Optional_depends");
		dep.erase(std::remove_if(dep.begin(), dep.end(),
				static_cast<int (*)(int)>(&std::isspace)), dep.end());
		for (const auto &dependency : str_split(dep, ',')) {
			spec.optdepends.insert(dependency);
		}
	}

	// Fallback to depends.txt
	if (!mod_conf_has_depends) {
		std::vector<std::string> dependencies;

		std::ifstream is((spec.path + DIR_DELIM + "depends.txt").c_str());

		if (is.good())
			spec.deprecation_msgs.push_back("depends.txt is deprecated, please use mod.conf instead.");

		while (is.good()) {
			std::string dep;
			std::getline(is, dep);
			dependencies.push_back(dep);
		}

		for (auto &dependency : dependencies) {
			std::unordered_set<char> symbols;
			if (parseDependsString(dependency, symbols)) {
				if (symbols.count('?') != 0) {
					spec.optdepends.insert(dependency);
				} else {
					spec.depends.insert(dependency);
				}
			}
		}
	}

	return true;
}

std::map<std::string, ModSpec> getModsInPath(
		const std::string &path, const std::string &virtual_path, int modpack_depth)
{
	// NOTE: this function works in mutual recursion with parseModContents

	std::map<std::string, ModSpec> result;
	std::vector<fs::DirListNode> dirlist = fs::GetDirListing(path);
	std::string mod_path;
	std::string mod_virtual_path;

	for (const fs::DirListNode &dln : dirlist) {
		if (!dln.dir)
			continue;

		const std::string &modname = dln.name;
		// Ignore all directories beginning with a ".", especially
		// VCS directories like ".git" or ".svn"
		if (modname[0] == '.')
			continue;

		mod_path.clear();
		mod_path.append(path).append(DIR_DELIM).append(modname);
		mod_path = fs::AbsolutePath(mod_path);

		mod_virtual_path.clear();
		// Intentionally uses / to keep paths same on different platforms
		mod_virtual_path.append(virtual_path).append("/").append(modname);

		// Support Part IV: Package Distribution & Library Structures
		// A library package contains a library.conf configuration file in its root directory
		// and groups all packages inside a directory named Library/
		if (fs::IsFile(mod_path + DIR_DELIM + "library.conf")) {
			std::string library_dir = mod_path + DIR_DELIM + "Library";
			if (fs::PathExists(library_dir)) {
				std::map<std::string, ModSpec> library_mods = getModsInPath(
						library_dir, mod_virtual_path + "/Library", modpack_depth + 1);
				for (auto &pair : library_mods) {
					result[pair.first] = std::move(pair.second);
				}
			}
			continue;
		}

		ModSpec spec(modname, mod_path, modpack_depth, mod_virtual_path);
		if (parseModContents(spec)) {
			result[modname] = std::move(spec);
		}
	}
	return result;
}

std::vector<ModSpec> flattenMods(const std::map<std::string, ModSpec> &mods,
		bool discard_modpacks)
{
	std::vector<ModSpec> result;
	for (const auto &it : mods) {
		const ModSpec &mod = it.second;
		if (!mod.is_modpack || !discard_modpacks) {
			result.push_back(mod);
		}
		if (mod.is_modpack) {
			std::vector<ModSpec> content = flattenMods(mod.modpack_content, discard_modpacks);
			result.reserve(result.size() + content.size());
			result.insert(result.end(), content.begin(), content.end());
		}
	}
	return result;
}


ModStorage::ModStorage(const std::string &mod_name, ModStorageDatabase *database):
	m_mod_name(mod_name), m_database(database)
{
}

void ModStorage::clear()
{
	m_database->removeModEntries(m_mod_name);
}

bool ModStorage::contains(const std::string &name) const
{
	return m_database->hasModEntry(m_mod_name, name);
}

bool ModStorage::setString(const std::string &name, std::string_view var)
{
	if (var.empty())
		return m_database->removeModEntry(m_mod_name, name);
	else
		return m_database->setModEntry(m_mod_name, name, var);
}

const StringMap &ModStorage::getStrings(StringMap *place) const
{
	place->clear();
	m_database->getModEntries(m_mod_name, place);
	return *place;
}

const std::vector<std::string> &ModStorage::getKeys(std::vector<std::string> *place) const
{
	place->clear();
	m_database->getModKeys(m_mod_name, place);
	return *place;
}

const std::string *ModStorage::getStringRaw(const std::string &name, std::string *place) const
{
	return m_database->getModEntry(m_mod_name, name, place) ? place : nullptr;
}
