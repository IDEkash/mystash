// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <variant>
#include <atomic>
#include <tuple>
#include <type_traits>
#include "irrlichttypes.h"
#include "irr_v3d.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

class ScriptApiBase;

// Lua conversion helpers (forward declarations)
void push_to_lua(lua_State *L, float v);
void push_to_lua(lua_State *L, int v);
void push_to_lua(lua_State *L, u32 v);
void push_to_lua(lua_State *L, u64 v);
void push_to_lua(lua_State *L, bool v);
void push_to_lua(lua_State *L, const std::string &v);
void push_to_lua(lua_State *L, v3f v);
void push_to_lua(lua_State *L, v3s16 v);

float read_lua_float(lua_State *L, int index);
int read_lua_int(lua_State *L, int index);
u32 read_lua_u32(lua_State *L, int index);
u64 read_lua_u64(lua_State *L, int index);
bool read_lua_bool(lua_State *L, int index);
std::string read_lua_string(lua_State *L, int index);
v3f read_lua_v3f(lua_State *L, int index);
v3s16 read_lua_v3s16(lua_State *L, int index);

// Template for pushing multiple args
inline void push_args(lua_State *L) {}
template<typename T, typename... Args>
void push_args(lua_State *L, T first, Args... rest) {
	push_to_lua(L, first);
	push_args(L, rest...);
}

class IPropertyAccessor {
public:
	virtual ~IPropertyAccessor() = default;
	virtual void push(lua_State *L, void *instance) = 0;
	virtual bool set(lua_State *L, void *instance, int index) = 0;
};

class IHookHandler {
public:
	virtual ~IHookHandler() = default;
	virtual void add_hook(lua_State *L, int index) = 0;
	virtual void set_override(lua_State *L, int index) = 0;
	virtual int call_from_lua(lua_State *L) = 0;
};

struct RegistryEntry {
	std::string name;
	void *instance;
	std::map<std::string, std::unique_ptr<IPropertyAccessor>> properties;
	std::map<std::string, std::function<int(lua_State*)>> methods;
	std::map<std::string, std::unique_ptr<IHookHandler>> hooks;

	void push_property(lua_State *L, const std::string &prop_name);
	bool set_property(lua_State *L, const std::string &prop_name, int value_index);
	void add_hook(lua_State *L, const std::string &hook_name, int func_index);
	void set_override(lua_State *L, const std::string &hook_name, int func_index);
	int call_hook(lua_State *L, const std::string &hook_name);
};

class EngineRegistry {
public:
	static void set_script_api(ScriptApiBase *api) { m_script_api = api; }
	static ScriptApiBase* get_script_api() { return m_script_api; }

	static void expose_raw(const std::string &name, void *instance);

	template<typename T, typename R, typename ...Args>
	static void expose_method(const std::string &system_name, const std::string &method_name, R (T::*method)(Args...));

	template<typename T, typename V>
	static void expose_property(
		const std::string &system_name,
		const std::string &prop_name,
		V T::*field
	);

	template<typename ...Args>
	static void expose_hook(
		const std::string &system_name,
		const std::string &hook_name,
		std::function<void(Args...)> &hook_point
	);

	static RegistryEntry* get(const std::string &name);
	static std::vector<std::string> list();
	static void register_all_reflected();

private:
	static std::map<std::string, std::unique_ptr<RegistryEntry>> m_entries;
	static ScriptApiBase *m_script_api;
};

template<typename T, typename V>
class DirectPropertyAccessor : public IPropertyAccessor {
	V T::*m_field;
public:
	DirectPropertyAccessor(V T::*field) : m_field(field) {}
	void push(lua_State *L, void *instance) override {
		T *obj = static_cast<T*>(instance);
		push_to_lua(L, (V)(obj->*m_field));
	}
	bool set(lua_State *L, void *instance, int index) override {
		T *obj = static_cast<T*>(instance);
		V val;
		set_from_lua(L, index, val);
		obj->*m_field = val;
		return true;
	}
private:
	void set_from_lua(lua_State *L, int index, float &v) { v = read_lua_float(L, index); }
	void set_from_lua(lua_State *L, int index, int &v) { v = read_lua_int(L, index); }
	void set_from_lua(lua_State *L, int index, u32 &v) { v = read_lua_u32(L, index); }
	void set_from_lua(lua_State *L, int index, u64 &v) { v = read_lua_u64(L, index); }
	void set_from_lua(lua_State *L, int index, bool &v) { v = read_lua_bool(L, index); }
	void set_from_lua(lua_State *L, int index, std::string &v) { v = read_lua_string(L, index); }
	void set_from_lua(lua_State *L, int index, v3f &v) { v = read_lua_v3f(L, index); }
	void set_from_lua(lua_State *L, int index, v3s16 &v) { v = read_lua_v3s16(L, index); }
};

template<typename T, typename V>
void EngineRegistry::expose_property(const std::string &system_name, const std::string &prop_name, V T::*field)
{
	RegistryEntry *entry = get(system_name);
	if (!entry) return;
	entry->properties[prop_name] = std::make_unique<DirectPropertyAccessor<T, V>>(field);
}

template<typename ...Args>
class HookHandler : public IHookHandler {
	std::function<void(Args...)> *m_hook_point;
	std::function<void(Args...)> m_original;
	std::vector<int> m_lua_hooks;
	int m_lua_override = -1;

public:
	HookHandler(std::function<void(Args...)> *hook_point) : m_hook_point(hook_point) {
		if (m_hook_point)
			m_original = *hook_point;
		update_hook_point();
	}

	void add_hook(lua_State *L, int index) override {
		lua_pushvalue(L, index);
		m_lua_hooks.push_back(luaL_ref(L, LUA_REGISTRYINDEX));
	}

	void set_override(lua_State *L, int index) override {
		if (m_lua_override != -1) {
			luaL_unref(L, LUA_REGISTRYINDEX, m_lua_override);
		}
		lua_pushvalue(L, index);
		m_lua_override = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	int call_from_lua(lua_State *L) override {
		return 0;
	}

	void run_chain(Args... args) {
		ScriptApiBase *api = EngineRegistry::get_script_api();
		if (!api) {
			if (m_original) m_original(args...);
			return;
		}
		lua_State *L = (lua_State*)api->getStack();

		if (m_lua_override != -1) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, m_lua_override);
			lua_pushlightuserdata(L, this);
			lua_pushcclosure(L, [](lua_State *L) {
				return 0;
			}, 1);
			push_args(L, args...);
			if (lua_pcall(L, 1 + sizeof...(Args), 0, 0) != 0) {
				lua_pop(L, 1);
			}
			return;
		}

		for (int ref : m_lua_hooks) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			push_args(L, args...);
			if (lua_pcall(L, sizeof...(Args), 1, 0) == 0) {
				if (lua_isboolean(L, -1) && !lua_toboolean(L, -1)) {
					lua_pop(L, 1);
					return;
				}
				lua_pop(L, 1);
			} else {
				lua_pop(L, 1);
			}
		}

		if (m_original) m_original(args...);
	}

private:
	void update_hook_point() {
		if (!m_hook_point) return;
		*m_hook_point = [this](Args... args) { run_chain(args...); };
	}
};

template<typename ...Args>
void EngineRegistry::expose_hook(const std::string &system_name, const std::string &hook_name, std::function<void(Args...)> &hook_point)
{
	RegistryEntry *entry = get(system_name);
	if (!entry) return;
	entry->hooks[hook_name] = std::make_unique<HookHandler<Args...>>(&hook_point);
}

// Helper to read argument from Lua by index
template<typename T> T read_arg(lua_State *L, int index);
template<> inline std::string read_arg<std::string>(lua_State *L, int index) { return read_lua_string(L, index); }
template<> inline int read_arg<int>(lua_State *L, int index) { return read_lua_int(L, index); }

template<typename T, typename R, typename ...Args>
void EngineRegistry::expose_method(const std::string &system_name, const std::string &method_name, R (T::*method)(Args...))
{
	RegistryEntry *entry = get(system_name);
	if (!entry) return;
	entry->methods[method_name] = [method, entry](lua_State *L) -> int {
		T *obj = static_cast<T*>(entry->instance);
		if (!obj) return 0;

		auto call_and_push = [&](auto... args) {
			if constexpr (std::is_void_v<R>) {
				(obj->*method)(args...);
				return 0;
			} else {
				R res = (obj->*method)(args...);
				push_to_lua(L, res);
				return 1;
			}
		};

		if constexpr (sizeof...(Args) == 0) {
			return call_and_push();
		} else if constexpr (sizeof...(Args) == 1) {
			using Arg1 = typename std::tuple_element<0, std::tuple<Args...>>::type;
			return call_and_push(read_arg<std::decay_t<Arg1>>(L, 1));
		} else if constexpr (sizeof...(Args) == 2) {
			using Arg1 = typename std::tuple_element<0, std::tuple<Args...>>::type;
			using Arg2 = typename std::tuple_element<1, std::tuple<Args...>>::type;
			return call_and_push(read_arg<std::decay_t<Arg1>>(L, 1), read_arg<std::decay_t<Arg2>>(L, 2));
		}
		return 0;
	};
}
