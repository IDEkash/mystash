// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_myengine.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "log.h"
#include "server.h"
#include "remoteplayer.h"
#include "server/player_sao.h"
#include "serverenvironment.h"
#include "scripting_server.h"
#include "myengine_registry.generated.h"
#include "settings.h"
#include "filesys.h"
#include <variant>

std::map<std::string, std::vector<int>> hooks_before;
std::map<std::string, std::vector<int>> hooks_after;
std::map<std::string, int> modifies;
std::map<std::string, int> rewrites;

static lua_State* g_L = nullptr;
lua_State* ModApiMyEngine::getLuaState() { return g_L; }

int ModApiMyEngine::l_get(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);
	std::vector<std::string> parts;
	size_t start = 0, end;
	while ((end = path.find('.', start)) != std::string::npos) {
		parts.push_back(path.substr(start, end - start));
		start = end + 1;
	}
	parts.push_back(path.substr(start));

	if (parts.empty()) {
		lua_pushnil(L);
		return 1;
	}

	void* obj = nullptr;
	std::string class_name;

	if (parts[0] == "world") {
		obj = &ModApiBase::getServer(L)->getEnv();
		class_name = "serverenvironment";
		parts.erase(parts.begin());
	} else if (parts[0] == "network") {
		obj = ModApiBase::getServer(L);
		class_name = "server";
		parts.erase(parts.begin());
	} else if (parts[0] == "player") {
		if (parts.size() >= 3) {
			RemotePlayer *player = ModApiBase::getServer(L)->getEnv().getPlayer(parts[1]);
			if (player) {
				obj = player->getPlayerSAO();
				class_name = "playersao";
				parts.erase(parts.begin(), parts.begin() + 2);
			}
		} else {
			lua_pushnil(L);
			return 1;
		}
	}

	if (obj && !class_name.empty()) {
		auto it = MyEngine::metadata.find(class_name);
		if (it != MyEngine::metadata.end()) {
			for (size_t i = 0; i < parts.size(); ++i) {
				auto mem_it = it->second.members.find(parts[i]);
				if (mem_it == it->second.members.end()) break;
				void* ptr = mem_it->second.get_ptr ? mem_it->second.get_ptr(obj) : nullptr;
				if (!ptr) break;
				if (i == parts.size() - 1) {
					if (mem_it->second.type == "u16") lua_pushinteger(L, *(u16*)ptr);
					else if (mem_it->second.type == "u32") lua_pushinteger(L, *(u32*)ptr);
					else if (mem_it->second.type == "int" || mem_it->second.type == "s32") lua_pushinteger(L, *(int*)ptr);
					else if (mem_it->second.type == "float" || mem_it->second.type == "f32") lua_pushnumber(L, *(float*)ptr);
					else if (mem_it->second.type == "std::string") lua_pushstring(L, ((std::string*)ptr)->c_str());
					else if (mem_it->second.type == "v3f") {
						v3f *v = (v3f*)ptr;
						lua_newtable(L);
						lua_pushnumber(L, v->X); lua_setfield(L, -2, "x");
						lua_pushnumber(L, v->Y); lua_setfield(L, -2, "y");
						lua_pushnumber(L, v->Z); lua_setfield(L, -2, "z");
					}
					else lua_pushlightuserdata(L, ptr);
					return 1;
				}
				obj = ptr;
				std::string next_class = mem_it->second.type;
				next_class.erase(std::remove(next_class.begin(), next_class.end(), '*'), next_class.end());
				next_class.erase(std::remove(next_class.begin(), next_class.end(), '&'), next_class.end());
				std::transform(next_class.begin(), next_class.end(), next_class.begin(), ::tolower);
				it = MyEngine::metadata.find(next_class);
				if (it == MyEngine::metadata.end()) break;
			}
		}
	}

	lua_pushnil(L);
	return 1;
}

static std::map<std::string, std::variant<u16, u32, int, float, std::string>> original_values;
static std::map<std::string, std::string> persistent_overrides;

static void save_overrides(lua_State *L) {
	std::string path = ModApiBase::getServer(L)->getWorldPath() + DIR_DELIM + "myengine_overrides.conf";
	Settings s;
	for (const auto& [k, v] : persistent_overrides) s.set(k, v);
	s.updateConfigFile(path.c_str());
}

int ModApiMyEngine::l_set(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);
	std::vector<std::string> parts;
	size_t start = 0, end;
	while ((end = path.find('.', start)) != std::string::npos) {
		parts.push_back(path.substr(start, end - start));
		start = end + 1;
	}
	parts.push_back(path.substr(start));

	if (parts.size() < 2) return 0;

	void* obj = nullptr;
	std::string class_name;
	if (parts[0] == "world") {
		obj = &ModApiBase::getServer(L)->getEnv();
		class_name = "serverenvironment";
		parts.erase(parts.begin());
	} else if (parts[0] == "player") {
		RemotePlayer *player = ModApiBase::getServer(L)->getEnv().getPlayer(parts[1]);
		if (player) {
			obj = player->getPlayerSAO();
			class_name = "playersao";
			parts.erase(parts.begin(), parts.begin() + 2);
		}
	}

	if (obj && !class_name.empty()) {
		auto it = MyEngine::metadata.find(class_name);
		if (it != MyEngine::metadata.end()) {
			for (size_t i = 0; i < parts.size(); ++i) {
				auto mem_it = it->second.members.find(parts[i]);
				if (mem_it == it->second.members.end()) break;
				void* ptr = mem_it->second.get_ptr ? mem_it->second.get_ptr(obj) : nullptr;
				if (!ptr) break;
				if (i == parts.size() - 1) {
					if (original_values.find(path) == original_values.end()) {
						if (mem_it->second.type == "u16") original_values[path] = *(u16*)ptr;
						else if (mem_it->second.type == "u32") original_values[path] = *(u32*)ptr;
						else if (mem_it->second.type == "int" || mem_it->second.type == "s32") original_values[path] = *(int*)ptr;
						else if (mem_it->second.type == "float" || mem_it->second.type == "f32") original_values[path] = *(float*)ptr;
						else if (mem_it->second.type == "std::string") original_values[path] = *(std::string*)ptr;
					}
					if (mem_it->second.type == "u16") *(u16*)ptr = (u16)luaL_checkinteger(L, 2);
					else if (mem_it->second.type == "u32") *(u32*)ptr = (u32)luaL_checkinteger(L, 2);
					else if (mem_it->second.type == "int" || mem_it->second.type == "s32") *(int*)ptr = (int)luaL_checkinteger(L, 2);
					else if (mem_it->second.type == "float" || mem_it->second.type == "f32") *(float*)ptr = (float)luaL_checknumber(L, 2);
					else if (mem_it->second.type == "std::string") *(std::string*)ptr = luaL_checkstring(L, 2);
					if (lua_toboolean(L, 3)) { persistent_overrides[path] = luaL_checkstring(L, 2); save_overrides(L); }
					return 0;
				}
				obj = ptr;
				std::string next_class = mem_it->second.type;
				next_class.erase(std::remove(next_class.begin(), next_class.end(), '*'), next_class.end());
				next_class.erase(std::remove(next_class.begin(), next_class.end(), '&'), next_class.end());
				std::transform(next_class.begin(), next_class.end(), next_class.begin(), ::tolower);
				it = MyEngine::metadata.find(next_class);
				if (it == MyEngine::metadata.end()) break;
			}
		}
	}
	return 0;
}

int ModApiMyEngine::l_hook(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	std::string position = luaL_checkstring(L, 3);
	lua_pushvalue(L, 2);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	if (position == "before") hooks_before[path].push_back(ref);
	else hooks_after[path].push_back(ref);
	return 0;
}

int ModApiMyEngine::l_modify(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	lua_pushvalue(L, 2);
	modifies[path] = luaL_ref(L, LUA_REGISTRYINDEX);
	return 0;
}

int ModApiMyEngine::l_rewrite(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_pushvalue(L, 2);
	rewrites[path] = luaL_ref(L, LUA_REGISTRYINDEX);
	return 0;
}

int ModApiMyEngine::l_add(lua_State *L)
{
	std::string type = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	lua_getglobal(L, "core");
	std::string func_name = "register_" + type;

	// Special case for entity
	if (type == "entity") {
		lua_getfield(L, -1, "register_entity");
		lua_getfield(L, 2, "name");
		std::string name = luaL_checkstring(L, -1);
		lua_pop(L, 1);
		lua_pushstring(L, name.c_str());
		lua_pushvalue(L, 2);
		lua_call(L, 2, 1);
		return 1;
	}

	lua_getfield(L, -1, func_name.c_str());
	if (lua_isfunction(L, -1)) {
		lua_pushvalue(L, 2);
		lua_call(L, 1, 1);
		return 1;
	}

	lua_pop(L, 2);
	lua_pushboolean(L, false);
	return 1;
}

int ModApiMyEngine::l_remove(lua_State *L)
{
	std::string type = luaL_checkstring(L, 1);
	std::string name = luaL_checkstring(L, 2);
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_items");
	lua_pushstring(L, name.c_str());
	lua_pushnil(L);
	lua_settable(L, -3);
	infostream << "MyEngine: Removed " << name << " from " << type << " registry" << std::endl;
	return 0;
}

int ModApiMyEngine::l_update(lua_State *L)
{
	return 0;
}

int ModApiMyEngine::l_watch(lua_State *L)
{
	return 0;
}

bool ModApiMyEngine::hasRewrite(const std::string &path) {
	return rewrites.find(path) != rewrites.end();
}

int ModApiMyEngine::getRewrite(const std::string &path) {
	auto it = rewrites.find(path);
	return (it != rewrites.end()) ? it->second : -1;
}

int ModApiMyEngine::getModify(const std::string &path) {
	auto it = modifies.find(path);
	return (it != modifies.end()) ? it->second : -1;
}

void ModApiMyEngine::resetAll() {
	for (const auto& [path, val] : original_values) {
		infostream << "MyEngine: Reverting " << path << std::endl;
		// Normally we would use the metadata and path resolution logic here
		// to set the value back. For now, we clear the map to stop the session overrides.
	}
	original_values.clear();
	hooks_before.clear();
	hooks_after.clear();
	modifies.clear();
	rewrites.clear();
}

void ModApiMyEngine::Initialize(lua_State *L, int top)
{
	g_L = L;
	MyEngine::registerAll();
	lua_newtable(L);
	int myengine_table = lua_gettop(L);
	registerFunction(L, "get", l_get, myengine_table);
	registerFunction(L, "set", l_set, myengine_table);
	registerFunction(L, "hook", l_hook, myengine_table);
	registerFunction(L, "modify", l_modify, myengine_table);
	registerFunction(L, "rewrite", l_rewrite, myengine_table);
	registerFunction(L, "add", l_add, myengine_table);
	registerFunction(L, "remove", l_remove, myengine_table);
	registerFunction(L, "update", l_update, myengine_table);
	registerFunction(L, "watch", l_watch, myengine_table);
	lua_setglobal(L, "myengine");
}
