// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_meta.h"
#include "engine/registry.h"
#include "script/lua_bridge.h"
#include <lua.hpp>

void ModApiMeta::Initialize(lua_State *L, int top) {
	lua_getglobal(L, "core");

	lua_newtable(L); // core.engine
	lua_pushcfunction(L, l_engine_get);
	lua_setfield(L, -2, "get");
	lua_pushcfunction(L, l_engine_list);
	lua_setfield(L, -2, "list");
	lua_setfield(L, -2, "engine");

	lua_pushcfunction(L, l_hook);
	lua_setfield(L, -2, "hook");

	lua_pushcfunction(L, l_override);
	lua_setfield(L, -2, "override");

	lua_pop(L, 1); // pop core
}

int ModApiMeta::l_engine_get(lua_State *L) {
	const char *name = luaL_checkstring(L, 1);
	RegistryEntry *entry = EngineRegistry::get(name);
	if (entry) {
		LuaBridge::build_table(L, entry);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ModApiMeta::l_engine_list(lua_State *L) {
	std::vector<std::string> names = EngineRegistry::list();
	lua_newtable(L);
	for (size_t i = 0; i < names.size(); ++i) {
		lua_pushstring(L, names[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

int ModApiMeta::l_hook(lua_State *L) {
	const char *full_name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	std::string s(full_name);
	size_t dot = s.find('.');
	if (dot == std::string::npos) return 0;

	std::string system_name = s.substr(0, dot);
	std::string hook_name = s.substr(dot + 1);

	RegistryEntry *entry = EngineRegistry::get(system_name);
	if (entry) {
		entry->add_hook(L, hook_name, 2);
	}
	return 0;
}

int ModApiMeta::l_override(lua_State *L) {
	const char *full_name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	std::string s(full_name);
	size_t dot = s.find('.');
	if (dot == std::string::npos) return 0;

	std::string system_name = s.substr(0, dot);
	std::string hook_name = s.substr(dot + 1);

	RegistryEntry *entry = EngineRegistry::get(system_name);
	if (entry) {
		entry->set_override(L, hook_name, 2);
	}
	return 0;
}
