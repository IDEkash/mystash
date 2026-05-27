// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_dim.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "cpp_api/s_security.h"
#include "content/subgames.h"
#include "settings.h"
#include "filesys.h"
#include "porting.h"
#include "map_settings_manager.h"
#include "server.h"
#include "log.h"
#include "util/string.h"
#include "convert_json.h"
#include <json/json.h>

int ModApiDim::l_create_world(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	const char *gameid = luaL_checkstring(L, 2);

	std::string path = porting::path_user + DIR_DELIM "worlds" + DIR_DELIM + sanitizeDirName(name, "world_");

	std::vector<SubgameSpec> games = getAvailableGames();
	auto game_it = std::find_if(games.begin(), games.end(), [gameid](const SubgameSpec &spec) {
		return spec.id == gameid;
	});

	if (game_it == games.end()) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Game ID not found");
		return 2;
	}

	StringMap use_settings;
	if (lua_istable(L, 3)) {
		lua_pushnil(L);
		while (lua_next(L, 3) != 0) {
			use_settings[luaL_checkstring(L, -2)] = luaL_checkstring(L, -1);
			lua_pop(L, 1);
		}
	}

	try {
		loadGameConfAndInitWorld(path, name, *game_it, true);

		// If there are extra settings (like mapgen), we need to update world.mt or map_meta.txt
		if (!use_settings.empty()) {
			std::string worldmt_path = path + DIR_DELIM "world.mt";
			Settings worldmt;
			worldmt.readConfigFile(worldmt_path.c_str());
			for (const auto &it : use_settings) {
				worldmt.set(it.first, it.second);
			}
			worldmt.updateConfigFile(worldmt_path.c_str());
		}

		lua_pushboolean(L, true);
		lua_pushstring(L, path.c_str());
		return 2;
	} catch (const BaseException &e) {
		lua_pushboolean(L, false);
		lua_pushstring(L, e.what());
		return 2;
	}
}

int ModApiDim::l_delete_world(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string path = luaL_checkstring(L, 1);

	// Security check: only allow deleting worlds in the worlds folder
	std::string worlds_dir = fs::AbsolutePath(porting::path_user + DIR_DELIM "worlds");
	std::string abs_path = fs::AbsolutePath(path);

	if (abs_path.find(worlds_dir) != 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Access denied: cannot delete outside of worlds directory");
		return 2;
	}

	// Don't allow deleting the current world
	if (abs_path == fs::AbsolutePath(getServer(L)->getWorldPath())) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Cannot delete currently active world");
		return 2;
	}

	if (!fs::RecursiveDelete(path)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to delete world directory");
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int ModApiDim::l_enter_world(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string path = luaL_checkstring(L, 1);

	if (!fs::PathExists(path)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "World path does not exist");
		return 2;
	}

	getServer(L)->requestWorldSwitch(path);
	lua_pushboolean(L, true);
	return 1;
}

int ModApiDim::l_set_visible(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string path = luaL_checkstring(L, 1);
	bool visible = readParam<bool>(L, 2);

	std::string worldmt_path = path + DIR_DELIM "world.mt";
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

int ModApiDim::l_link_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string world_path = luaL_checkstring(L, 1);
	std::string mod_path = luaL_checkstring(L, 2);

	CHECK_SECURE_PATH(L, mod_path.c_str(), false);

	std::string world_meta_path = world_path + DIR_DELIM "worldmeta.json";
	Json::Value root;

	std::ifstream is(world_meta_path, std::ifstream::binary);
	if (is.good()) {
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		std::string errs;
		if (!Json::parseFromStream(builder, is, &root, &errs)) {
			root = Json::objectValue;
		}
	} else {
		root = Json::objectValue;
	}
	is.close();

	if (!root.isMember("linked_mods") || !root["linked_mods"].isArray()) {
		root["linked_mods"] = Json::arrayValue;
	}

	// Check if already linked
	for (const auto &item : root["linked_mods"]) {
		if (item.asString() == mod_path) {
			lua_pushboolean(L, true);
			return 1;
		}
	}

	root["linked_mods"].append(mod_path);

	Json::StreamWriterBuilder builder;
	std::string out = Json::writeString(builder, root);

	if (!fs::safeWriteToFile(world_meta_path, out)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write worldmeta.json");
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

void ModApiDim::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int tbl = lua_gettop(L);

	registerFunction(L, "create_world", l_create_world, tbl);
	registerFunction(L, "delete_world", l_delete_world, tbl);
	registerFunction(L, "enter_world", l_enter_world, tbl);
	registerFunction(L, "set_visible", l_set_visible, tbl);
	registerFunction(L, "link_mod", l_link_mod, tbl);

	lua_pushvalue(L, tbl);
	lua_setglobal(L, "dimension");

	lua_setfield(L, top, "dimension");
}
