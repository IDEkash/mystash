// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "irrlichttypes.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

class IPropertyAccessor {
public:
	virtual ~IPropertyAccessor() = default;
	virtual void push(lua_State *L, void *instance) = 0;
	virtual void set(lua_State *L, void *instance, int index) = 0;
};

class IHookHandler {
public:
	virtual ~IHookHandler() = default;
	virtual void add_hook(lua_State *L, int index) = 0;
	virtual void set_override(lua_State *L, int index) = 0;
};

struct RegistryEntry {
	std::string name;
	void *instance;
	std::map<std::string, std::unique_ptr<IPropertyAccessor>> properties;
	std::map<std::string, std::unique_ptr<IHookHandler>> hooks;

	void push_property(lua_State *L, const std::string &prop_name);
	void set_property(lua_State *L, const std::string &prop_name, int value_index);
	void add_hook(lua_State *L, const std::string &hook_name, int func_index);
	void set_override(lua_State *L, const std::string &hook_name, int func_index);
};

class EngineRegistry {
public:
	static void expose_raw(const std::string &name, void *instance);

	template<typename T, typename V>
	static void expose_property(
		const std::string &system_name,
		const std::string &prop_name,
		V T::*field
	);

	template<typename T, typename ...Args>
	static void expose_hook(
		const std::string &system_name,
		const std::string &hook_name,
		std::function<void(Args...)> &hook_point
	);

	static RegistryEntry* get(const std::string &name);
	static std::vector<std::string> list();

private:
	static std::map<std::string, std::unique_ptr<RegistryEntry>> m_entries;
};

// Helper templates for Lua conversion (to be implemented in registry.cpp or a separate header)
void push_to_lua(lua_State *L, float v);
void push_to_lua(lua_State *L, int v);
void push_to_lua(lua_State *L, u32 v);
void push_to_lua(lua_State *L, u64 v);
void push_to_lua(lua_State *L, bool v);
void push_to_lua(lua_State *L, const std::string &v);
float read_float(lua_State *L, int index);
int read_int(lua_State *L, int index);
u32 read_u32(lua_State *L, int index);
u64 read_u64(lua_State *L, int index);
bool read_bool(lua_State *L, int index);
std::string read_string(lua_State *L, int index);

template<typename T, typename V>
class PropertyAccessor : public IPropertyAccessor {
	V T::*m_field;
public:
	PropertyAccessor(V T::*field) : m_field(field) {}
	void push(lua_State *L, void *instance) override {
		T *obj = static_cast<T*>(instance);
		push_to_lua(L, obj->*m_field);
	}
	void set(lua_State *L, void *instance, int index) override {
		T *obj = static_cast<T*>(instance);
		// This needs specialization or overloads for V
		set_from_lua(L, index, obj->*m_field);
	}

private:
	void set_from_lua(lua_State *L, int index, float &v) { v = read_float(L, index); }
	void set_from_lua(lua_State *L, int index, int &v) { v = read_int(L, index); }
	void set_from_lua(lua_State *L, int index, u32 &v) { v = read_u32(L, index); }
	void set_from_lua(lua_State *L, int index, u64 &v) { v = read_u64(L, index); }
	void set_from_lua(lua_State *L, int index, bool &v) { v = read_bool(L, index); }
	void set_from_lua(lua_State *L, int index, std::string &v) { v = read_string(L, index); }
};

template<typename T, typename V>
void EngineRegistry::expose_property(const std::string &system_name, const std::string &prop_name, V T::*field)
{
	RegistryEntry *entry = get(system_name);
	if (!entry) return;
	entry->properties[prop_name] = std::make_unique<PropertyAccessor<T, V>>(field);
}

template<typename ...Args>
class HookHandler : public IHookHandler {
	std::function<void(Args...)> *m_hook_point;
	std::function<void(Args...)> m_original;
	std::vector<int> m_lua_hooks;
	int m_lua_override = -1;

public:
	HookHandler(std::function<void(Args...)> *hook_point) : m_hook_point(hook_point) {
		m_original = *hook_point;
	}

	void add_hook(lua_State *L, int index) override {
		lua_pushvalue(L, index);
		m_lua_hooks.push_back(luaL_ref(L, LUA_REGISTRYINDEX));
		update_hook_point(L);
	}

	void set_override(lua_State *L, int index) override {
		if (m_lua_override != -1) {
			luaL_unref(L, LUA_REGISTRYINDEX, m_lua_override);
		}
		lua_pushvalue(L, index);
		m_lua_override = luaL_ref(L, LUA_REGISTRYINDEX);
		update_hook_point(L);
	}

private:
	void update_hook_point(lua_State *L) {
		// This is a simplification. Real implementation needs to handle Lua callback from C++
		// which requires access to lua_State.
	}
};

template<typename T, typename ...Args>
void EngineRegistry::expose_hook(const std::string &system_name, const std::string &hook_name, std::function<void(Args...)> &hook_point)
{
	RegistryEntry *entry = get(system_name);
	if (!entry) return;
	entry->hooks[hook_name] = std::make_unique<HookHandler<Args...>>(&hook_point);
}
