// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "engine/registry.h"
#include <lua.h>
#include <lauxlib.h>

std::map<std::string, std::unique_ptr<RegistryEntry>> EngineRegistry::m_entries;
ScriptApiBase *EngineRegistry::m_script_api = nullptr;

void EngineRegistry::expose_raw(const std::string &name, void *instance) {
	auto it = m_entries.find(name);
	if (it == m_entries.end()) {
		auto entry = std::make_unique<RegistryEntry>();
		entry->name = name;
		entry->instance = instance;
		m_entries[name] = std::move(entry);
	} else {
		m_entries[name]->instance = instance;
	}
}

RegistryEntry* EngineRegistry::get(const std::string &name) {
	auto it = m_entries.find(name);
	if (it != m_entries.end()) {
		return it->second.get();
	}
	// Create entry even if instance is null for now (lazy binding)
	auto entry = std::make_unique<RegistryEntry>();
	entry->name = name;
	entry->instance = nullptr;
	RegistryEntry *ptr = entry.get();
	m_entries[name] = std::move(entry);
	return ptr;
}

std::vector<std::string> EngineRegistry::list() {
	std::vector<std::string> names;
	for (auto const& [name, entry] : m_entries) {
		names.push_back(name);
	}
	return names;
}

void RegistryEntry::push_property(lua_State *L, const std::string &prop_name) {
	if (!instance) {
		lua_pushnil(L);
		return;
	}
	auto it = properties.find(prop_name);
	if (it != properties.end()) {
		it->second->push(L, instance);
	} else {
		auto it_method = methods.find(prop_name);
		if (it_method != methods.end()) {
			lua_pushstring(L, prop_name.c_str());
			lua_pushlightuserdata(L, this);
			lua_pushcclosure(L, [](lua_State *L) {
				RegistryEntry *entry = (RegistryEntry *)lua_touserdata(L, lua_upvalueindex(2));
				const char *m_name = lua_tostring(L, lua_upvalueindex(1));
				return entry->methods[m_name](L);
			}, 2);
			return;
		}
		auto it_hook = hooks.find(prop_name);
		if (it_hook != hooks.end()) {
			lua_pushstring(L, prop_name.c_str());
			lua_pushlightuserdata(L, this);
			lua_pushcclosure(L, [](lua_State *L) {
				RegistryEntry *entry = (RegistryEntry *)lua_touserdata(L, lua_upvalueindex(2));
				const char *hook_name = lua_tostring(L, lua_upvalueindex(1));
				return entry->call_hook(L, hook_name);
			}, 2);
		} else {
			lua_pushnil(L);
		}
	}
}

bool RegistryEntry::set_property(lua_State *L, const std::string &prop_name, int value_index) {
	if (!instance) return false;
	auto it = properties.find(prop_name);
	if (it != properties.end()) {
		return it->second->set(L, instance, value_index);
	}
	return false;
}

void RegistryEntry::add_hook(lua_State *L, const std::string &hook_name, int func_index) {
	auto it = hooks.find(hook_name);
	if (it != hooks.end()) {
		it->second->add_hook(L, func_index);
	}
}

void RegistryEntry::set_override(lua_State *L, const std::string &hook_name, int func_index) {
	auto it = hooks.find(hook_name);
	if (it != hooks.end()) {
		it->second->set_override(L, func_index);
	}
}

int RegistryEntry::call_hook(lua_State *L, const std::string &hook_name) {
	auto it = hooks.find(hook_name);
	if (it != hooks.end()) {
		return it->second->call_from_lua(L);
	}
	return 0;
}

// Lua conversion helpers
void push_to_lua(lua_State *L, float v) { lua_pushnumber(L, v); }
void push_to_lua(lua_State *L, int v) { lua_pushinteger(L, v); }
void push_to_lua(lua_State *L, u32 v) { lua_pushinteger(L, v); }
void push_to_lua(lua_State *L, u64 v) { lua_pushinteger(L, (lua_Integer)v); }
void push_to_lua(lua_State *L, bool v) { lua_pushboolean(L, v); }
void push_to_lua(lua_State *L, const std::string &v) { lua_pushstring(L, v.c_str()); }
void push_to_lua(lua_State *L, v3f v) {
	lua_newtable(L);
	lua_pushnumber(L, v.X); lua_setfield(L, -2, "x");
	lua_pushnumber(L, v.Y); lua_setfield(L, -2, "y");
	lua_pushnumber(L, v.Z); lua_setfield(L, -2, "z");
}
void push_to_lua(lua_State *L, v3s16 v) {
	lua_newtable(L);
	lua_pushinteger(L, v.X); lua_setfield(L, -2, "x");
	lua_pushinteger(L, v.Y); lua_setfield(L, -2, "y");
	lua_pushinteger(L, v.Z); lua_setfield(L, -2, "z");
}

float read_lua_float(lua_State *L, int index) { return (float)lua_tonumber(L, index); }
int read_lua_int(lua_State *L, int index) { return (int)lua_tointeger(L, index); }
u32 read_lua_u32(lua_State *L, int index) { return (u32)lua_tointeger(L, index); }
u64 read_lua_u64(lua_State *L, int index) { return (u64)lua_tointeger(L, index); }
bool read_lua_bool(lua_State *L, int index) { return lua_toboolean(L, index) != 0; }
v3f read_lua_v3f(lua_State *L, int index) {
	v3f v;
	lua_getfield(L, index, "x"); v.X = lua_tonumber(L, -1); lua_pop(L, 1);
	lua_getfield(L, index, "y"); v.Y = lua_tonumber(L, -1); lua_pop(L, 1);
	lua_getfield(L, index, "z"); v.Z = lua_tonumber(L, -1); lua_pop(L, 1);
	return v;
}
v3s16 read_lua_v3s16(lua_State *L, int index) {
	v3s16 v;
	lua_getfield(L, index, "x"); v.X = lua_tointeger(L, -1); lua_pop(L, 1);
	lua_getfield(L, index, "y"); v.Y = lua_tointeger(L, -1); lua_pop(L, 1);
	lua_getfield(L, index, "z"); v.Z = lua_tointeger(L, -1); lua_pop(L, 1);
	return v;
}
std::string read_lua_string(lua_State *L, int index) {
	const char *s = lua_tostring(L, index);
	return s ? s : "";
}
