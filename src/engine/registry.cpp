// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "engine/registry.h"

std::map<std::string, std::unique_ptr<RegistryEntry>> EngineRegistry::m_entries;

void EngineRegistry::expose_raw(const std::string &name, void *instance) {
	if (m_entries.find(name) == m_entries.end()) {
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
		lua_pushnil(L);
	}
}

void RegistryEntry::set_property(lua_State *L, const std::string &prop_name, int value_index) {
	if (!instance) return;
	auto it = properties.find(prop_name);
	if (it != properties.end()) {
		it->second->set(L, instance, value_index);
	}
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

// Lua conversion helpers
void push_to_lua(lua_State *L, float v) { lua_pushnumber(L, v); }
void push_to_lua(lua_State *L, int v) { lua_pushinteger(L, v); }
void push_to_lua(lua_State *L, u32 v) { lua_pushinteger(L, v); }
void push_to_lua(lua_State *L, u64 v) { lua_pushinteger(L, (lua_Integer)v); }
void push_to_lua(lua_State *L, bool v) { lua_pushboolean(L, v); }
void push_to_lua(lua_State *L, const std::string &v) { lua_pushstring(L, v.c_str()); }

float read_float(lua_State *L, int index) { return (float)lua_tonumber(L, index); }
int read_int(lua_State *L, int index) { return (int)lua_tointeger(L, index); }
u32 read_u32(lua_State *L, int index) { return (u32)lua_tointeger(L, index); }
u64 read_u64(lua_State *L, int index) { return (u64)lua_tointeger(L, index); }
bool read_bool(lua_State *L, int index) { return lua_toboolean(L, index); }
std::string read_string(lua_State *L, int index) {
	const char *s = lua_tostring(L, index);
	return s ? s : "";
}
