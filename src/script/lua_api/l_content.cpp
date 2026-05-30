// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 sapier
// Copyright (C) 2025 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "lua_api/l_content.h"
#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "content/subgames.h"
#include "mapgen/mapgen.h"
#include "filesys.h"
#include "porting.h"
#include "settings.h"
#include "server.h"
#include "log.h"
#ifdef CLIENT
#include "client/texturepaths.h"
#endif

int ModApiContent::l_get_worlds(lua_State *L)
{
	std::vector<WorldSpec> worlds = getAvailableWorlds();

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (const WorldSpec &world : worlds) {
		lua_pushnumber(L, index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L, "path");
		lua_pushstring(L, world.path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "name");
		lua_pushstring(L, world.name.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "gameid");
		lua_pushstring(L, world.gameid.c_str());
		lua_settable(L, top_lvl2);

		lua_settable(L, top);
		index++;
	}
	return 1;
}

int ModApiContent::l_get_games(lua_State *L)
{
	std::vector<SubgameSpec> games = getAvailableGames();

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (const SubgameSpec &game : games) {
		lua_pushnumber(L, index);
		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L, "id");
		lua_pushstring(L, game.id.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "path");
		lua_pushstring(L, game.path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "type");
		lua_pushstring(L, "game");
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "gamemods_path");
		lua_pushstring(L, game.gamemods_path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "name");
		lua_pushstring(L, game.title.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "title");
		lua_pushstring(L, game.title.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "author");
		lua_pushstring(L, game.author.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "release");
		lua_pushinteger(L, game.release);
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "menuicon_path");
#ifdef CLIENT
		auto menuicon = getImagePath(game.path + DIR_DELIM "menu" DIR_DELIM "icon.png");
		lua_pushstring(L, menuicon.c_str());
#else
		lua_pushstring(L, (game.path + DIR_DELIM "menu" DIR_DELIM "icon.png").c_str());
#endif
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "addon_mods_paths");
		lua_newtable(L);
		int table2 = lua_gettop(L);
		int internal_index = 1;
		for (const auto &addon_mods_path : game.addon_mods_paths) {
			lua_pushnumber(L, internal_index);
			lua_pushstring(L, addon_mods_path.second.c_str());
			lua_settable(L, table2);
			internal_index++;
		}
		lua_settable(L, top_lvl2);
		lua_settable(L, top);
		index++;
	}
	return 1;
}

int ModApiContent::l_get_mapgen_names(lua_State *L)
{
	std::vector<const char *> names;
	bool include_hidden = lua_isboolean(L, 1) && readParam<bool>(L, 1);
	Mapgen::getMapgenNames(&names, include_hidden);

	lua_newtable(L);
	for (size_t i = 0; i != names.size(); i++) {
		lua_pushstring(L, names[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

int ModApiContent::l_create_world(lua_State *L)
{
	std::string worldname;
	std::string gameid;
	StringMap initial_settings;
	std::vector<ModSpec> mods_to_add;

	if (lua_istable(L, 1)) {
		// New API: core.create_world(def)
		worldname = getstringfield_default(L, 1, "world_name", "");
		gameid = getstringfield_default(L, 1, "game_id", "");

		if (worldname.empty()) {
			lua_pushboolean(L, false);
			lua_pushstring(L, "world_name is required");
			return 2;
		}
		if (gameid.empty()) {
			lua_pushboolean(L, false);
			lua_pushstring(L, "game_id is required");
			return 2;
		}

		std::string seed = getstringfield_default(L, 1, "seed", "");
		if (!seed.empty())
			initial_settings["seed"] = seed;

		std::string mg_name = getstringfield_default(L, 1, "mg_name", "");
		if (!mg_name.empty())
			initial_settings["mg_name"] = mg_name;

		std::string mg_flags = getstringfield_default(L, 1, "mg_flags", "");
		if (!mg_flags.empty())
			initial_settings["mg_flags"] = mg_flags;

		std::string mg_spflags = getstringfield_default(L, 1, "mg_spflags", "");
		if (!mg_spflags.empty()) {
			// If mg_name is provided, we can be more specific
			if (!mg_name.empty())
				initial_settings["mg" + mg_name + "_spflags"] = mg_spflags;
			else {
				// Otherwise set it for all common ones as a fallback or if not using specific ones
				initial_settings["mgv5_spflags"] = mg_spflags;
				initial_settings["mgv6_spflags"] = mg_spflags;
				initial_settings["mgv7_spflags"] = mg_spflags;
				initial_settings["mgcarpathian_spflags"] = mg_spflags;
				initial_settings["mgvalleys_spflags"] = mg_spflags;
				initial_settings["mgflat_spflags"] = mg_spflags;
				initial_settings["mgfractal_spflags"] = mg_spflags;
			}
		}

		lua_getfield(L, 1, "settings");
		if (lua_istable(L, -1)) {
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				if (lua_isstring(L, -2)) {
					std::string key = readParam<std::string>(L, -2);
					std::string val;
					if (lua_isboolean(L, -1))
						val = lua_toboolean(L, -1) ? "true" : "false";
					else
						val = readParam<std::string>(L, -1);
					initial_settings[key] = val;
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		lua_getfield(L, 1, "mods");
		if (lua_istable(L, -1)) {
			int table_len = lua_objlen(L, -1);
			for (int i = 1; i <= table_len; i++) {
				lua_rawgeti(L, -1, i);
				if (lua_isstring(L, -1)) {
					std::string modname = readParam<std::string>(L, -1);
					// Find mod in global paths
					bool found = false;
					for (const auto &p : getEnvModPaths()) {
						std::string path = p + DIR_DELIM + modname;
						if (fs::IsDir(path)) {
							ModSpec spec(modname, path);
							mods_to_add.push_back(spec);
							found = true;
							break;
						}
					}
					if (!found) {
						std::string path = porting::path_user + DIR_DELIM "mods" + DIR_DELIM + modname;
						if (fs::IsDir(path)) {
							ModSpec spec(modname, path);
							mods_to_add.push_back(spec);
							found = true;
						}
					}
					if (!found) {
						std::string path = porting::path_share + DIR_DELIM "mods" + DIR_DELIM + modname;
						if (fs::IsDir(path)) {
							ModSpec spec(modname, path);
							mods_to_add.push_back(spec);
							found = true;
						}
					}
					if (!found) {
						lua_pushboolean(L, false);
						lua_pushstring(L, ("Mod not found: " + modname).c_str());
						return 2;
					}
				} else if (lua_istable(L, -1)) {
					std::string modname = getstringfield_default(L, -1, "name", "");
					std::string modpath = getstringfield_default(L, -1, "path", "");
					bool is_worldmod = getboolfield_default(L, -1, "worldmod", false);
					if (modname.empty() || modpath.empty()) {
						lua_pushboolean(L, false);
						lua_pushstring(L, "Mod name and path required");
						return 2;
					}
					ModSpec spec(modname, modpath);
					if (is_worldmod)
						spec.virtual_path = "worldmods";
					mods_to_add.push_back(spec);
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

	} else {
		// Old API: core.create_world(name, gameid, settings)
		worldname = luaL_checkstring(L, 1);
		gameid = luaL_checkstring(L, 2);
		luaL_checktype(L, 3, LUA_TTABLE);
		lua_pushnil(L);
		while (lua_next(L, 3) != 0) {
			initial_settings[luaL_checkstring(L, -2)] = luaL_checkstring(L, -1);
			lua_pop(L, 1);
		}
	}

	std::string path = porting::path_user + DIR_DELIM "worlds" + DIR_DELIM +
			sanitizeDirName(worldname, "world_");

	SubgameSpec gamespec = findSubgame(gameid);
	if (!gamespec.isValid()) {
		if (lua_istable(L, 1)) {
			lua_pushboolean(L, false);
			lua_pushstring(L, "Game ID not found");
			return 2;
		} else {
			lua_pushstring(L, "Game ID not found");
			return 1;
		}
	}

	try {
		loadGameConfAndInitWorld(path, worldname, gamespec, true, initial_settings);

		// Handle mods
		if (!mods_to_add.empty()) {
			std::string worldmt_path = path + DIR_DELIM "world.mt";
			Settings worldmt;
			worldmt.readConfigFile(worldmt_path.c_str());

			for (const auto &mod : mods_to_add) {
				std::string final_mod_path = mod.path;
				if (mod.virtual_path == "worldmods") {
					std::string dest = path + DIR_DELIM "worldmods" + DIR_DELIM + mod.name;
					fs::CreateAllDirs(path + DIR_DELIM "worldmods");
					if (!fs::CopyDir(mod.path, dest)) {
						throw BaseException("Failed to copy mod " + mod.name + " to worldmods");
					}
					final_mod_path = dest;
				}
				worldmt.set("load_mod_" + mod.name, final_mod_path);
			}
			worldmt.updateConfigFile(worldmt_path.c_str());
		}

		if (lua_istable(L, 1)) {
			lua_pushboolean(L, true);
			return 1;
		} else {
			lua_pushnil(L);
			return 1;
		}
	} catch (const BaseException &e) {
		std::string err = std::string("Failed to initialize world: ") + e.what();
		if (lua_istable(L, 1)) {
			lua_pushboolean(L, false);
			lua_pushstring(L, err.c_str());
			return 2;
		} else {
			lua_pushstring(L, err.c_str());
			return 1;
		}
	}
}

int ModApiContent::l_delete_world(lua_State *L)
{
	std::string worldname;
	int world_idx = -1;

	if (lua_isnumber(L, 1)) {
		world_idx = lua_tointeger(L, 1) - 1;
	} else if (lua_isstring(L, 1)) {
		worldname = lua_tostring(L, 1);
	} else {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Invalid argument: world name or index expected");
		return 2;
	}

	std::vector<WorldSpec> worlds = getAvailableWorlds();
	const WorldSpec *spec = nullptr;

	if (world_idx != -1) {
		if (world_idx >= 0 && world_idx < (int)worlds.size())
			spec = &worlds[world_idx];
	} else {
		for (const auto &w : worlds) {
			if (w.name == worldname) {
				spec = &w;
				break;
			}
		}
	}

	if (!spec) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "World not found");
		return 2;
	}

	// Check if world is currently running
	Server *server = getServer(L);
	if (server && fs::AbsolutePath(server->getWorldPath()) == fs::AbsolutePath(spec->path)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Cannot delete currently running world");
		return 2;
	}

	if (!fs::RecursiveDelete(spec->path)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to delete world directory");
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int ModApiContent::l_get_game_settings(lua_State *L)
{
	std::string gameid = luaL_checkstring(L, 1);
	SubgameSpec gamespec = findSubgame(gameid);
	if (!gamespec.isValid()) {
		lua_pushnil(L);
		lua_pushstring(L, "Game ID not found");
		return 2;
	}

	std::string settingtypes_path = gamespec.path + DIR_DELIM + "settingtypes.txt";
	if (!fs::PathExists(settingtypes_path)) {
		lua_newtable(L);
		return 1;
	}

	// We'll use the existing Lua parser in builtin/common/settings/settingtypes.lua
	// To do this, we'll call into Lua.
	// Since we are in a C++ API called from Lua, we can just use the global state.

	lua_getglobal(L, "settingtypes");
	if (!lua_istable(L, -1)) {
		// If it's not available yet (e.g. during startup), we might need to load it.
		// But in mainmenu it should be there. In server it might not be.
		lua_pop(L, 1);
		std::string builtin_path = porting::path_share + DIR_DELIM "builtin" DIR_DELIM "common" DIR_DELIM "settings" DIR_DELIM "settingtypes.lua";
		if (luaL_dofile(L, builtin_path.c_str())) {
			const char *err = lua_tostring(L, -1);
			errorstream << "Failed to load settingtypes.lua: " << (err ? err : "unknown error") << std::endl;
			lua_pushnil(L);
			lua_pushstring(L, "Failed to load settingtypes parser");
			return 2;
		}
		lua_getglobal(L, "settingtypes");
	}

	lua_getfield(L, -1, "parse_config_file");
	if (!lua_isfunction(L, -1)) {
		lua_pushnil(L);
		lua_pushstring(L, "settingtypes.parse_config_file not found");
		return 2;
	}

	// Signature: settingtypes.parse_config_file(read_all, parse_mods)
	lua_pushboolean(L, true); // read_all
	lua_pushboolean(L, false); // parse_mods (we only want this game's settings)

	// But wait, parse_config_file parses EVERYTHING if parse_mods is true,
	// or only builtin if parse_mods is false.
	// It doesn't take a path.
	// I should probably add a function to settingtypes that takes a path.

	// Let's check settingtypes.lua again.
	// It has parse_single_file(file, filepath, read_all, result, base_level, allow_secure, force_context)
	// but it's local.

	// I will just implement a simple wrapper in Lua if needed, or call parse_single_file if I can make it global.
	// Actually, I can just use io.open and call a Lua helper.

	lua_pop(L, 3); // pop function and args and settingtypes table

	// Let's create a Lua helper on the fly or use an existing one.
	// Actually, I'll just use luaL_loadstring to create a helper that uses the existing settingtypes.lua logic.

	const char *lua_helper =
		"local game_path = ...\n"
		"local settings = {}\n"
		"local path = game_path .. '/settingtypes.txt'\n"
		"local file = io.open(path, 'r')\n"
		"if not file then return {} end\n"
		"local builtin_path = core.get_builtin_path() .. 'common/settings/settingtypes.lua'\n"
		"if not settingtypes then dofile(builtin_path) end\n"
		"-- We need access to parse_single_file which is local in settingtypes.lua\n"
		"-- This is a bit tricky. Maybe I should just expose it or use another way.\n"
		"-- For now, let's assume I can call a modified version or just re-implement the call.\n"
		"-- Actually, settingtypes.parse_config_file(true, true) will parse all games.\n"
		"-- We can then just filter for the one we want.\n"
		"local all_settings = settingtypes.parse_config_file(true, true)\n"
		"local result = {}\n"
		"local found_game = false\n"
		"for _, s in ipairs(all_settings) do\n"
		"  if s.type == 'category' and s.level == 1 and s.name == game_path then\n"
		"    found_game = true\n"
		"  elseif s.type == 'category' and s.level <= 1 then\n"
		"    found_game = false\n"
		"  elseif found_game then\n"
		"    table.insert(result, s)\n"
		"  end\n"
		"end\n"
		"return result\n";

	if (luaL_loadstring(L, lua_helper)) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to load Lua helper");
		return 2;
	}

	lua_pushstring(L, gamespec.path.c_str());
	if (lua_pcall(L, 1, 1, 0)) {
		const char *err = lua_tostring(L, -1);
		lua_pushnil(L);
		lua_pushstring(L, err);
		return 2;
	}

	// Now we have the raw settings table from Lua at top of stack.
	// We need to map it to the requested format:
	// name, type, default, label (readable_name), comment, values (for enum/flags), min, max

	int data_idx = lua_gettop(L);
	lua_newtable(L); // Final result table
	int res_idx = lua_gettop(L);
	int i = 1;

	lua_pushnil(L);
	while (lua_next(L, data_idx)) {
		// key at index -2 and value at index -1
		if (lua_istable(L, -1)) {
			lua_newtable(L);
			int s_idx = lua_gettop(L);

			// Copy fields and rename where necessary
			lua_getfield(L, -2, "name");
			lua_setfield(L, s_idx, "name");

			lua_getfield(L, -2, "type");
			lua_setfield(L, s_idx, "type");

			lua_getfield(L, -2, "default");
			lua_setfield(L, s_idx, "default");

			lua_getfield(L, -2, "readable_name");
			lua_setfield(L, s_idx, "label");

			lua_getfield(L, -2, "comment");
			lua_setfield(L, s_idx, "comment");

			lua_getfield(L, -2, "values");
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				lua_getfield(L, -2, "possible");
			}
			lua_setfield(L, s_idx, "values");

			lua_getfield(L, -2, "min");
			lua_setfield(L, s_idx, "min");

			lua_getfield(L, -2, "max");
			lua_setfield(L, s_idx, "max");

			lua_rawseti(L, res_idx, i++);
		}
		lua_pop(L, 1); // pop value, keep key for next iteration
	}

	return 1;
}

void ModApiContent::Initialize(lua_State *L, int top)
{
	API_FCT(get_worlds);
	API_FCT(get_games);
	API_FCT(get_mapgen_names);
	API_FCT(create_world);
	API_FCT(delete_world);
	API_FCT(get_game_settings);
}

void ModApiContent::InitializeAsync(lua_State *L, int top)
{
	API_FCT(get_worlds);
	API_FCT(get_games);
	API_FCT(get_mapgen_names);
}
