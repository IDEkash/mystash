// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "script/lua_bridge.h"
#include "engine/registry.h"

static int l_index_metamethod(lua_State *L) {
	RegistryEntry *entry = (RegistryEntry *)lua_touserdata(L, lua_upvalueindex(1));
	const char *key = luaL_checkstring(L, 2);
	entry->push_property(L, key);
	return 1;
}

static int l_newindex_metamethod(lua_State *L) {
	RegistryEntry *entry = (RegistryEntry *)lua_touserdata(L, lua_upvalueindex(1));
	const char *key = luaL_checkstring(L, 2);
	if (!entry->set_property(L, key, 3)) {
		luaL_error(L, "Cannot write to field '%s' (not a property of system '%s')", key, entry->name.c_str());
	}
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
