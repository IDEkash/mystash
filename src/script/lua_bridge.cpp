// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "script/lua_bridge.h"
#include "engine/registry.h"
#include <lua.hpp>

static int l_index_metamethod(lua_State *L) {
	RegistryEntry *entry = (RegistryEntry *)lua_touserdata(L, lua_upvalueindex(1));
	const char *key = luaL_checkstring(L, 2);
	entry->push_property(L, key);
	return 1;
}

static int l_newindex_metamethod(lua_State *L) {
	RegistryEntry *entry = (RegistryEntry *)lua_touserdata(L, lua_upvalueindex(1));
	const char *key = luaL_checkstring(L, 2);
	entry->set_property(L, key, 3);
	return 0;
}

void LuaBridge::build_table(lua_State *L, RegistryEntry *entry) {
	lua_newtable(L);

	lua_newtable(L); // Metatable
	lua_pushlightuserdata(L, entry);
	lua_pushcclosure(L, l_index_metamethod, 1);
	lua_setfield(L, -2, "__index");

	lua_pushlightuserdata(L, entry);
	lua_pushcclosure(L, l_newindex_metamethod, 1);
	lua_setfield(L, -2, "__newindex");

	lua_setmetatable(L, -2);
}
