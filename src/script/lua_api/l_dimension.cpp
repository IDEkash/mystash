// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_dimension.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"
#include "filesys.h"
#include "porting.h"
#include "settings.h"
#include "content/subgames.h"
#include "util/string.h"
#include "convert_json.h"

int ModApiDimension::l_create(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	luaL_checktype(L, 1, LUA_TTABLE);

	std::string world_name = getstringfield_default(L, 1, "world_name", "");
	if (world_name.empty()) {
		lua_pushnil(L);
		lua_pushstring(L, "world_name is required");
		return 2;
	}

	std::string gameid = getstringfield_default(L, 1, "gameid", "");
	if (gameid.empty()) {
		gameid = getGameDef(L)->getGameSpec()->id;
	}

	std::string path = porting::path_user + DIR_DELIM "worlds" + DIR_DELIM + sanitizeDirName(world_name, "world_");
	if (fs::PathExists(path)) {
		lua_pushnil(L);
		lua_pushstring(L, "World already exists");
		return 2;
	}

	SubgameSpec gamespec = findSubgame(gameid);
	if (!gamespec.isValid()) {
		lua_pushnil(L);
		lua_pushstring(L, "Game ID not found");
		return 2;
	}

	StringMap use_settings;
	// Extract mapgen settings
	lua_pushnil(L);
	while (lua_next(L, 1) != 0) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			std::string key = lua_tostring(L, -2);
			if (key == "seed") {
				use_settings["fixed_map_seed"] = lua_tostring(L, -1);
			} else if (key == "mg_name") {
				use_settings["mg_name"] = lua_tostring(L, -1);
			} else if (key == "mg_flags") {
				use_settings["mg_flags"] = lua_tostring(L, -1);
			} else if (str_starts_with(key, "mg") && key.find("_spflags") != std::string::npos) {
				use_settings[key] = lua_tostring(L, -1);
			}
		}
		lua_pop(L, 1);
	}

	StringMap backup;
	for (auto &it : use_settings) {
		if (g_settings->existsLocal(it.first))
			backup[it.first] = g_settings->get(it.first);
		g_settings->set(it.first, it.second);
	}

	bool success = false;
	std::string error_msg;

	try {
		loadGameConfAndInitWorld(path, world_name, gamespec, true);

		std::string worldmt_path = path + DIR_DELIM "world.mt";
		Settings worldmt;
		worldmt.readConfigFile(worldmt_path.c_str());

		bool hidden = getboolfield_default(L, 1, "hidden", false);
		if (hidden) {
			worldmt.setBool("visible", false);
		}

		// Handle mods table
		lua_getfield(L, 1, "mods");
		if (lua_istable(L, -1)) {
			Json::Value linked_mods(Json::arrayValue);
			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				std::string mod = luaL_checkstring(L, -1);
				if (mod.find('/') != std::string::npos || mod.find('\\') != std::string::npos) {
					// Path
					linked_mods.append(fs::AbsolutePath(mod));
				} else {
					// Mod name
					worldmt.set("load_mod_" + mod, "true");
				}
				lua_pop(L, 1);
			}

			if (!linked_mods.empty()) {
				std::string worldmeta_path = path + DIR_DELIM "worldmeta.json";
				Json::Value root;
				root["linked_mods"] = linked_mods;
				std::ofstream os(worldmeta_path, std::ios::binary);
				os << root;
			}
		}
		lua_pop(L, 1);

		worldmt.updateConfigFile(worldmt_path.c_str());

		// Trigger callback
		lua_getglobal(L, "core");
		lua_getfield(L, -1, "run_callbacks");
		lua_getglobal(L, "core");
		lua_getfield(L, -1, "registered_on_dimension_createds");
		lua_pushstring(L, path.c_str());
		lua_pushinteger(L, 0); // mode 0
		if (lua_pcall(L, 3, 0, 0) != 0) {
			errorstream << "Error running on_dimension_created callbacks: " << lua_tostring(L, -1) << std::endl;
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		success = true;
	} catch (const BaseException &e) {
		error_msg = e.what();
	}

	for (auto &it : use_settings) {
		auto it2 = backup.find(it.first);
		if (it2 == backup.end())
			g_settings->remove(it.first);
		else
			g_settings->set(it.first, it2->second);
	}

	if (success) {
		lua_pushstring(L, path.c_str());
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushstring(L, error_msg.c_str());
		return 2;
	}
}

int ModApiDimension::l_delete(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string path = luaL_checkstring(L, 1);

	std::string worlds_dir = fs::AbsolutePath(porting::path_user + DIR_DELIM "worlds");
	std::string absolute_path = fs::AbsolutePath(path);

	if (!str_starts_with(absolute_path, worlds_dir)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Cannot delete worlds outside the worlds directory");
		return 2;
	}

	if (absolute_path == fs::AbsolutePath(getGameDef(L)->getWorldPath())) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Cannot delete the currently active world");
		return 2;
	}

	if (!fs::RecursiveDelete(path)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to delete world");
		return 2;
	}

	// Trigger callback
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "run_callbacks");
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_dimension_deleteds");
	lua_pushstring(L, path.c_str());
	lua_pushinteger(L, 0);
	if (lua_pcall(L, 3, 0, 0) != 0) {
		errorstream << "Error running on_dimension_deleted callbacks: " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	lua_pushboolean(L, true);
	return 1;
}

int ModApiDimension::l_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::vector<WorldSpec> worlds = getAvailableWorlds();
	lua_newtable(L);
	for (size_t i = 0; i < worlds.size(); i++) {
		lua_pushstring(L, worlds[i].path.c_str());
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

int ModApiDimension::l_exists(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string path = luaL_checkstring(L, 1);
	lua_pushboolean(L, getWorldExists(path));
	return 1;
}

int ModApiDimension::l_get_info(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string path = luaL_checkstring(L, 1);
	if (!getWorldExists(path)) {
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);
	setstringfield(L, -1, "path", path);
	setstringfield(L, -1, "name", getWorldName(path, ""));
	setstringfield(L, -1, "gameid", getWorldGameId(path, true));

	std::string worldmt_path = path + DIR_DELIM "world.mt";
	Settings worldmt;
	if (worldmt.readConfigFile(worldmt_path.c_str())) {
		lua_pushboolean(L, worldmt.getBool("visible", true));
		lua_setfield(L, -2, "visible");
	}

	return 1;
}

int ModApiDimension::l_transfer_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	// Arg 1 is player (ignored for now as we switch everyone/the server) or path
	std::string path;
	if (lua_isstring(L, 2)) {
		path = lua_tostring(L, 2);
	} else {
		path = luaL_checkstring(L, 1);
	}

	if (!getWorldExists(path)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Invalid world path");
		return 2;
	}

	getServer(L)->requestWorldSwitch(path);
	lua_pushboolean(L, true);
	return 1;
}

int ModApiDimension::l_set_visible(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string world_path = luaL_checkstring(L, 1);
	bool visible = readParam<bool>(L, 2);

	std::string worldmt_path = world_path + DIR_DELIM "world.mt";
	Settings worldmt;
	if (!worldmt.readConfigFile(worldmt_path.c_str())) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Could not read world.mt");
		return 2;
	}

	worldmt.setBool("visible", visible);
	if (!worldmt.updateConfigFile(worldmt_path.c_str())) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Could not update world.mt");
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int ModApiDimension::l_link_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string world_path = luaL_checkstring(L, 1);
	std::string mod_path = luaL_checkstring(L, 2);

	std::string worldmeta_path = world_path + DIR_DELIM "worldmeta.json";
	Json::Value root;
	if (fs::PathExists(worldmeta_path)) {
		std::ifstream is(worldmeta_path, std::ios::binary);
		is >> root;
	}

	if (!root.isObject())
		root = Json::Value(Json::objectValue);

	if (!root["linked_mods"].isArray())
		root["linked_mods"] = Json::Value(Json::arrayValue);

	std::string abs_mod_path = fs::AbsolutePath(mod_path);
	bool found = false;
	for (const auto &m : root["linked_mods"]) {
		if (m.asString() == abs_mod_path) {
			found = true;
			break;
		}
	}

	if (!found) {
		root["linked_mods"].append(abs_mod_path);
		std::ofstream os(worldmeta_path, std::ios::binary);
		os << root;
	}

	lua_pushboolean(L, true);
	return 1;
}

void ModApiDimension::Initialize(lua_State *L, int top)
{
	registerFunction(L, "create", l_create, top);
	registerFunction(L, "delete", l_delete, top);
	registerFunction(L, "list", l_list, top);
	registerFunction(L, "exists", l_exists, top);
	registerFunction(L, "get_info", l_get_info, top);
	registerFunction(L, "transfer_player", l_transfer_player, top);
	registerFunction(L, "enter_world", l_transfer_player, top); // alias
	registerFunction(L, "load", l_transfer_player, top); // alias
	registerFunction(L, "unload", l_transfer_player, top); // alias
	registerFunction(L, "set_visible", l_set_visible, top);
	registerFunction(L, "link_mod", l_link_mod, top);
}
