#include "myengine.h"
#include "aliases.h"
#include "log.h"
#include "lua_api/l_base.h"
#include "common/c_converter.h"
#include <string>
#include <vector>
#include <sstream>

namespace MyEngine {

static std::vector<std::string> split_path(const std::string &path) {
	std::vector<std::string> parts;
	std::stringstream ss(path);
	std::string part;
	while (std::getline(ss, part, '.')) {
		if (!part.empty()) {
			parts.push_back(part);
		}
	}
	return parts;
}

static bool push_nested_value(lua_State *L, const std::string &path) {
	std::vector<std::string> parts = split_path(path);
	if (parts.empty()) return false;

	lua_getglobal(L, parts[0].c_str());
	for (size_t i = 1; i < parts.size(); ++i) {
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			return false;
		}
		lua_getfield(L, -1, parts[i].c_str());
		lua_remove(L, -2);
	}
	return true;
}

static int l_get(lua_State *L) {
	std::string path = luaL_checkstring(L, 1);
	std::string resolved = g_myengine_aliases.resolve(path);

	if (!push_nested_value(L, resolved)) {
		warningstream << "myengine.get: Path not found: " << resolved << " (aliased from: " << path << ")" << std::endl;
		lua_pushnil(L);
	}
	return 1;
}

static int l_set(lua_State *L) {
	std::string path = luaL_checkstring(L, 1);
	std::string resolved = g_myengine_aliases.resolve(path);
	std::vector<std::string> parts = split_path(resolved);

	if (parts.empty()) {
		warningstream << "myengine.set: Invalid path: " << resolved << std::endl;
		return 0;
	}

	if (parts.size() == 1) {
		lua_pushvalue(L, 2);
		lua_setglobal(L, parts[0].c_str());
	} else {
		lua_getglobal(L, parts[0].c_str());
		for (size_t i = 1; i < parts.size() - 1; ++i) {
			if (!lua_istable(L, -1)) {
				warningstream << "myengine.set: Path not found (not a table): " << parts[i] << " in " << resolved << std::endl;
				lua_pop(L, 1);
				return 0;
			}
			lua_getfield(L, -1, parts[i].c_str());
			lua_remove(L, -2);
		}
		if (!lua_istable(L, -1)) {
			warningstream << "myengine.set: Path not found (not a table): " << parts[parts.size() - 2] << " in " << resolved << std::endl;
			lua_pop(L, 1);
			return 0;
		}
		lua_pushvalue(L, 2);
		lua_setfield(L, -2, parts.back().c_str());
		lua_pop(L, 1);
	}
	return 0;
}

static int l_hook(lua_State *L) {
	std::string event_name = luaL_checkstring(L, 1);
	std::string resolved = g_myengine_aliases.resolve(event_name);

	// Try to use core.register_on_* for hooks
	std::string reg_func_name = "register_on_" + resolved;
	lua_getglobal(L, "core");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, reg_func_name.c_str());
		if (lua_isfunction(L, -1)) {
			lua_pushvalue(L, 2); // The callback
			if (lua_pcall(L, 1, 0, 0) != 0) {
				errorstream << "myengine.hook: Error calling " << reg_func_name << ": " << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		} else {
			warningstream << "myengine.hook: No such registration function: core." << reg_func_name << " (event: " << resolved << ")" << std::endl;
			lua_pop(L, 1); // pop nil/non-function
		}
	}
	lua_pop(L, 1); // pop core
	return 0;
}

static int l_modify(lua_State *L) {
	std::string type = luaL_checkstring(L, 1);
	std::string name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);

	// Implement modification logic (stub for now, as it depends on Minetest internals)
	infostream << "myengine.modify: Modifying " << type << " " << name << std::endl;

	// In Minetest, override_item is often used for nodes/items
	if (type == "node" || type == "item" || type == "craftitem") {
		lua_getglobal(L, "core");
		lua_getfield(L, -1, "override_item");
		if (lua_isfunction(L, -1)) {
			lua_pushstring(L, name.c_str());
			lua_pushvalue(L, 3);
			if (lua_pcall(L, 2, 0, 0) != 0) {
				errorstream << "myengine.modify: Error calling core.override_item: " << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		} else {
			lua_pop(L, 1); // pop nil/non-function
		}
		lua_pop(L, 1); // pop core
	}

	return 0;
}

static int l_add(lua_State *L) {
	std::string type = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	infostream << "myengine.add: Adding " << type << std::endl;

	std::string reg_func;
	if (type == "node") reg_func = "register_node";
	else if (type == "entity") reg_func = "register_entity";
	else if (type == "item") reg_func = "register_craftitem";
	else if (type == "tool") reg_func = "register_tool";

	if (!reg_func.empty()) {
		lua_getglobal(L, "core");
		lua_getfield(L, -1, reg_func.c_str());
		if (lua_isfunction(L, -1)) {
			// Get name from definition if possible, register_node(name, def)
			lua_getfield(L, 2, "name");
			if (lua_isstring(L, -1)) {
				std::string name = lua_tostring(L, -1);
				lua_pop(L, 1); // pop name string
				lua_pushstring(L, name.c_str());
				lua_pushvalue(L, 2);
				if (lua_pcall(L, 2, 0, 0) != 0) {
					errorstream << "myengine.add: Error calling core." << reg_func << ": " << lua_tostring(L, -1) << std::endl;
					lua_pop(L, 1);
				}
			} else {
				lua_pop(L, 1); // pop nil/non-string
				lua_pushvalue(L, 2);
				if (lua_pcall(L, 1, 0, 0) != 0) {
					errorstream << "myengine.add: Error calling core." << reg_func << ": " << lua_tostring(L, -1) << std::endl;
					lua_pop(L, 1);
				}
			}
		} else {
			lua_pop(L, 1); // pop nil/non-function
		}
		lua_pop(L, 1); // pop core
	}

	return 0;
}

static int l_remove(lua_State *L) {
	std::string type = luaL_checkstring(L, 1);
	std::string name = luaL_checkstring(L, 2);

	infostream << "myengine.remove: Removing " << type << " " << name << std::endl;

	lua_getglobal(L, "core");
	std::string unreg_func;
	if (type == "node") unreg_func = "unregister_item"; // Minetest uses unregister_item for nodes too
	else if (type == "item") unreg_func = "unregister_item";

	if (!unreg_func.empty()) {
		lua_getfield(L, -1, unreg_func.c_str());
		if (lua_isfunction(L, -1)) {
			lua_pushstring(L, name.c_str());
			if (lua_pcall(L, 1, 0, 0) != 0) {
				errorstream << "myengine.remove: Error calling core." << unreg_func << ": " << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		} else {
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	return 0;
}

void initialize(lua_State *L) {
	lua_newtable(L);
	int myengine_top = lua_gettop(L);

	auto register_func = [&](const char *name, lua_CFunction func) {
		lua_pushcfunction(L, func);
		lua_setfield(L, myengine_top, name);
	};

	register_func("get", l_get);
	register_func("set", l_set);
	register_func("hook", l_hook);
	register_func("modify", l_modify);
	register_func("add", l_add);
	register_func("remove", l_remove);

	lua_setglobal(L, "myengine");
}

}
