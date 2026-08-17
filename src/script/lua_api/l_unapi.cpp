// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "l_unapi.h"
#include "lua_api/l_internal.h"
#include "unapi_registry.h"
#include "unapi_hooks.h"
#include "unapi_storage.h"
#include "unapi_network.h"
#include "unapi_foundations.h"
#include "log.h"
#include "common/c_converter.h"

struct ObjectUserData {
	unapi::ObjectRefPtr obj;
};

#define UNAPI_OBJECT_METATABLE "unapi.object"

static unapi::ObjectRefPtr checkUnapiObject(lua_State *L, int index) {
	if (lua_isuserdata(L, index)) {
		ObjectUserData *ud = (ObjectUserData *)luaL_checkudata(L, index, UNAPI_OBJECT_METATABLE);
		if (ud && ud->obj) return ud->obj;
	} else if (lua_istable(L, index)) {
		lua_getfield(L, index, "_handle_id");
		if (lua_isnumber(L, -1)) {
			uint64_t handle = static_cast<uint64_t>(lua_tointeger(L, -1));
			lua_pop(L, 1);
			return unapi::UnapiRegistry::get().getObjectByHandle(handle);
		}
		lua_pop(L, 1);
	}
	return nullptr;
}

void ModApiUnapi::pushValueToLua(lua_State *L, const unapi::Value &val) {
	switch (val.type) {
		case unapi::ValueType::Nil:
			lua_pushnil(L);
			break;
		case unapi::ValueType::Boolean:
			lua_pushboolean(L, val.asBool() ? 1 : 0);
			break;
		case unapi::ValueType::Integer:
			lua_pushinteger(L, val.asInt());
			break;
		case unapi::ValueType::Float:
			lua_pushnumber(L, val.asFloat());
			break;
		case unapi::ValueType::String:
			lua_pushstring(L, val.asString().c_str());
			break;
		case unapi::ValueType::Vector3: {
			unapi::Vector3f v = val.asVector3();
			lua_createtable(L, 0, 3);
			lua_pushnumber(L, v.x);
			lua_setfield(L, -2, "x");
			lua_pushnumber(L, v.y);
			lua_setfield(L, -2, "y");
			lua_pushnumber(L, v.z);
			lua_setfield(L, -2, "z");
			break;
		}
		case unapi::ValueType::Vector2: {
			unapi::Vector2f v = val.asVector2();
			lua_createtable(L, 0, 2);
			lua_pushnumber(L, v.x);
			lua_setfield(L, -2, "x");
			lua_pushnumber(L, v.y);
			lua_setfield(L, -2, "y");
			break;
		}
		case unapi::ValueType::ObjectRef: {
			unapi::ObjectRefPtr obj = val.asObject();
			if (!obj) {
				lua_pushnil(L);
			} else {
				ObjectUserData *ud = (ObjectUserData *)lua_newuserdata(L, sizeof(ObjectUserData));
				new (ud) ObjectUserData();
				ud->obj = obj;
				luaL_getmetatable(L, UNAPI_OBJECT_METATABLE);
				lua_setmetatable(L, -2);
			}
			break;
		}
		case unapi::ValueType::Array: {
			auto arr = std::get<std::shared_ptr<unapi::ValueArray>>(val.data);
			lua_newtable(L);
			if (arr) {
				int idx = 1;
				for (const auto &item : *arr) {
					pushValueToLua(L, item);
					lua_rawseti(L, -2, idx++);
				}
			}
			break;
		}
		case unapi::ValueType::Map: {
			auto map = std::get<std::shared_ptr<unapi::ValueMap>>(val.data);
			lua_newtable(L);
			if (map) {
				for (const auto &pair : *map) {
					pushValueToLua(L, pair.second);
					lua_setfield(L, -2, pair.first.c_str());
				}
			}
			break;
		}
		default:
			lua_pushnil(L);
			break;
	}
}

unapi::Value ModApiUnapi::luaToValue(lua_State *L, int index) {
	int t = lua_type(L, index);
	if (t == LUA_TNIL || t == LUA_TNONE) {
		return unapi::Value();
	}
	if (t == LUA_TBOOLEAN) {
		return unapi::Value(lua_toboolean(L, index) != 0);
	}
	if (t == LUA_TNUMBER) {
		double d = lua_tonumber(L, index);
		int64_t i = lua_tointeger(L, index);
		if (static_cast<double>(i) == d) {
			return unapi::Value(i);
		}
		return unapi::Value(d);
	}
	if (t == LUA_TSTRING) {
		return unapi::Value(std::string(lua_tostring(L, index)));
	}
	if (t == LUA_TUSERDATA) {
		unapi::ObjectRefPtr obj = checkUnapiObject(L, index);
		if (obj) return unapi::Value(obj);
	}
	if (t == LUA_TTABLE) {
		// Check vector3 or vector2
		lua_getfield(L, index, "x");
		lua_getfield(L, index, "y");
		lua_getfield(L, index, "z");
		bool has_x = lua_isnumber(L, -3) != 0;
		bool has_y = lua_isnumber(L, -2) != 0;
		bool has_z = lua_isnumber(L, -1) != 0;

		if (has_x && has_y && has_z) {
			float x = static_cast<float>(lua_tonumber(L, -3));
			float y = static_cast<float>(lua_tonumber(L, -2));
			float z = static_cast<float>(lua_tonumber(L, -1));
			lua_pop(L, 3);
			return unapi::Value(unapi::Vector3f(x, y, z));
		}
		if (has_x && has_y && !has_z) {
			float x = static_cast<float>(lua_tonumber(L, -3));
			float y = static_cast<float>(lua_tonumber(L, -2));
			lua_pop(L, 3);
			return unapi::Value(unapi::Vector2f(x, y));
		}
		lua_pop(L, 3);

		// Check object wrapper table
		unapi::ObjectRefPtr obj = checkUnapiObject(L, index);
		if (obj) return unapi::Value(obj);

		// Check array vs map
		lua_pushnil(L);
		bool is_map = false;
		size_t arr_size = lua_objlen(L, index);
		while (lua_next(L, index) != 0) {
			if (lua_type(L, -2) != LUA_TNUMBER) {
				is_map = true;
				lua_pop(L, 2);
				break;
			}
			lua_pop(L, 1);
		}

		if (is_map) {
			unapi::ValueMap map;
			lua_pushnil(L);
			while (lua_next(L, index) != 0) {
				if (lua_type(L, -2) == LUA_TSTRING) {
					std::string k = lua_tostring(L, -2);
					map[k] = luaToValue(L, lua_gettop(L));
				}
				lua_pop(L, 1);
			}
			return unapi::Value(map);
		} else {
			unapi::ValueArray arr;
			for (size_t i = 1; i <= arr_size; i++) {
				lua_rawgeti(L, index, i);
				arr.push_back(luaToValue(L, -1));
				lua_pop(L, 1);
			}
			return unapi::Value(arr);
		}
	}
	return unapi::Value();
}

int ModApiUnapi::l_inspect(lua_State *L) {
	if (lua_isstring(L, 1)) {
		std::string type_name = lua_tostring(L, 1);
		unapi::ValueMap info = unapi::UnapiRegistry::get().inspectType(type_name);
		pushValueToLua(L, unapi::Value(info));
		return 1;
	}
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	if (obj) {
		unapi::ValueMap info = unapi::UnapiRegistry::get().inspectObject(obj);
		pushValueToLua(L, unapi::Value(info));
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

int ModApiUnapi::l_create(lua_State *L) {
	const char *type_name = luaL_checkstring(L, 1);
	std::string ext_id = "default";
	if (lua_isstring(L, 2)) {
		ext_id = lua_tostring(L, 2);
	}
	unapi::ObjectRefPtr obj = unapi::UnapiRegistry::get().createObject(type_name, ext_id);
	if (obj) {
		pushValueToLua(L, unapi::Value(obj));
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

int ModApiUnapi::l_resolve(lua_State *L) {
	if (lua_isnumber(L, 1)) {
		uint64_t handle = static_cast<uint64_t>(lua_tointeger(L, 1));
		unapi::ObjectRefPtr obj = unapi::UnapiRegistry::get().getObjectByHandle(handle);
		if (obj) {
			pushValueToLua(L, unapi::Value(obj));
			return 1;
		}
	} else if (lua_isstring(L, 1)) {
		std::string type_name = lua_tostring(L, 1);
		unapi::ValueMap info = unapi::UnapiRegistry::get().inspectType(type_name);
		pushValueToLua(L, unapi::Value(info));
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

int ModApiUnapi::l_hook(lua_State *L) {
	const char *hook_name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	std::string ext_id = "extension";
	unapi::HookTiming timing = unapi::HookTiming::Before;

	if (lua_istable(L, 3)) {
		lua_getfield(L, 3, "timing");
		if (lua_isstring(L, -1)) {
			std::string tstr = lua_tostring(L, -1);
			if (tstr == "after") timing = unapi::HookTiming::After;
			else if (tstr == "override") timing = unapi::HookTiming::Override;
		}
		lua_pop(L, 1);

		lua_getfield(L, 3, "extension_id");
		if (lua_isstring(L, -1)) {
			ext_id = lua_tostring(L, -1);
		}
		lua_pop(L, 1);
	}

	lua_pushvalue(L, 2);
	int lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	uint64_t hook_id = unapi::HookRegistry::get().registerHook(
		hook_name, ext_id, timing,
		[L, lua_ref](const unapi::ValueArray &args, bool &override_exec, unapi::Value &override_res) -> unapi::Value {
			lua_rawgeti(L, LUA_REGISTRYINDEX, lua_ref);
			for (const auto &arg : args) {
				pushValueToLua(L, arg);
			}
			int status = lua_pcall(L, static_cast<int>(args.size()), 2, 0);
			if (status != 0) {
				errorstream << "[UNAPI Hook Error] " << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
				return unapi::Value();
			}
			if (lua_toboolean(L, -1)) {
				override_exec = true;
			}
			unapi::Value ret = luaToValue(L, -2);
			if (override_exec) {
				override_res = ret;
			}
			lua_pop(L, 2);
			return ret;
		},
		lua_ref
	);

	lua_pushinteger(L, hook_id);
	return 1;
}

int ModApiUnapi::l_unhook(lua_State *L) {
	uint64_t hook_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
	bool ok = unapi::HookRegistry::get().unregisterHook(hook_id);
	lua_pushboolean(L, ok ? 1 : 0);
	return 1;
}

int ModApiUnapi::l_list_types(lua_State *L) {
	std::string filter = "";
	if (lua_isstring(L, 1)) filter = lua_tostring(L, 1);
	auto types = unapi::UnapiRegistry::get().listTypes(filter);
	lua_newtable(L);
	int idx = 1;
	for (const auto &t : types) {
		lua_pushstring(L, t.c_str());
		lua_rawseti(L, -2, idx++);
	}
	return 1;
}

int ModApiUnapi::l_list_hooks(lua_State *L) {
	auto hooks = unapi::HookRegistry::get().getRegisteredHooks();
	lua_newtable(L);
	int idx = 1;
	for (const auto &h : hooks) {
		lua_pushstring(L, h.c_str());
		lua_rawseti(L, -2, idx++);
	}
	return 1;
}

int ModApiUnapi::l_storage_get(lua_State *L) {
	const char *ext_id = luaL_checkstring(L, 1);
	auto storage = unapi::UnapiRegistry::get().getStorage(ext_id);

	lua_newtable(L);

	// storage:exists(path)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		const char *path = luaL_checkstring(L2, path_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		lua_pushboolean(L2, st->exists(path) ? 1 : 0);
		return 1;
	}, 1);
	lua_setfield(L, -2, "exists");

	// storage:create_dir(path)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		const char *path = luaL_checkstring(L2, path_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		lua_pushboolean(L2, st->createDirectory(path) ? 1 : 0);
		return 1;
	}, 1);
	lua_setfield(L, -2, "create_dir");

	// storage:read(path)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		const char *path = luaL_checkstring(L2, path_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		std::string content = st->readFile(path);
		lua_pushstring(L2, content.c_str());
		return 1;
	}, 1);
	lua_setfield(L, -2, "read");

	// storage:write(path, content)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		int content_idx = lua_istable(L2, 1) ? 3 : 2;
		const char *path = luaL_checkstring(L2, path_idx);
		const char *content = luaL_checkstring(L2, content_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		lua_pushboolean(L2, st->writeFile(path, content) ? 1 : 0);
		return 1;
	}, 1);
	lua_setfield(L, -2, "write");

	// storage:append(path, content)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		int content_idx = lua_istable(L2, 1) ? 3 : 2;
		const char *path = luaL_checkstring(L2, path_idx);
		const char *content = luaL_checkstring(L2, content_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		lua_pushboolean(L2, st->appendFile(path, content) ? 1 : 0);
		return 1;
	}, 1);
	lua_setfield(L, -2, "append");

	// storage:delete(path)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		const char *path = luaL_checkstring(L2, path_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		lua_pushboolean(L2, st->deleteFile(path) ? 1 : 0);
		return 1;
	}, 1);
	lua_setfield(L, -2, "delete");

	// storage:rename(old_path, new_path)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int opath_idx = lua_istable(L2, 1) ? 2 : 1;
		int npath_idx = lua_istable(L2, 1) ? 3 : 2;
		const char *opath = luaL_checkstring(L2, opath_idx);
		const char *npath = luaL_checkstring(L2, npath_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		lua_pushboolean(L2, st->renameFile(opath, npath) ? 1 : 0);
		return 1;
	}, 1);
	lua_setfield(L, -2, "rename");

	// storage:list(path)
	lua_pushstring(L, ext_id);
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		const char *ext = lua_tostring(L2, lua_upvalueindex(1));
		int path_idx = lua_istable(L2, 1) ? 2 : 1;
		std::string path = "";
		if (lua_isstring(L2, path_idx)) path = lua_tostring(L2, path_idx);
		auto st = unapi::UnapiRegistry::get().getStorage(ext);
		auto files = st->listDirectory(path);
		lua_newtable(L2);
		int idx = 1;
		for (const auto &f : files) {
			lua_pushstring(L2, f.c_str());
			lua_rawseti(L2, -2, idx++);
		}
		return 1;
	}, 1);
	lua_setfield(L, -2, "list");

	return 1;
}

int ModApiUnapi::l_network_http_fetch(lua_State *L) {
	const char *url = luaL_checkstring(L, 1);
	std::string method = "GET";
	std::string data = "";
	int timeout = 5000;
	unapi::ValueMap headers;
	std::string ext_id = "default";

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "method");
		if (lua_isstring(L, -1)) method = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "data");
		if (lua_isstring(L, -1)) data = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "timeout");
		if (lua_isnumber(L, -1)) timeout = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 2, "extension_id");
		if (lua_isstring(L, -1)) ext_id = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "headers");
		if (lua_istable(L, -1)) {
			unapi::Value hval = luaToValue(L, lua_gettop(L));
			if (hval.type == unapi::ValueType::Map) {
				headers = *std::get<std::shared_ptr<unapi::ValueMap>>(hval.data);
			}
		}
		lua_pop(L, 1);
	}

	if (!unapi::UnapiRegistry::get().hasPermission(ext_id, unapi::Permission::NetworkHttp)) {
		lua_newtable(L);
		lua_pushboolean(L, 0);
		lua_setfield(L, -2, "succeeded");
		lua_pushstring(L, "Permission network.http denied");
		lua_setfield(L, -2, "error");
		return 1;
	}

	unapi::HttpResponse resp = unapi::ExtensionNetwork::fetchHttp(ext_id, url, method, headers, data, timeout);
	lua_newtable(L);
	lua_pushinteger(L, resp.status_code);
	lua_setfield(L, -2, "status");
	lua_pushstring(L, resp.body.c_str());
	lua_setfield(L, -2, "body");
	lua_pushboolean(L, resp.completed ? 1 : 0);
	lua_setfield(L, -2, "succeeded");
	if (!resp.error.empty()) {
		lua_pushstring(L, resp.error.c_str());
		lua_setfield(L, -2, "error");
	}
	return 1;
}

// Object userdata methods
int ModApiUnapi::l_object_get_property(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	const char *prop = luaL_checkstring(L, 2);
	if (!obj) { lua_pushnil(L); return 1; }
	unapi::Value val = obj->getProperty(prop);
	pushValueToLua(L, val);
	return 1;
}

int ModApiUnapi::l_object_set_property(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	const char *prop = luaL_checkstring(L, 2);
	unapi::Value val = luaToValue(L, 3);
	if (!obj) { lua_pushboolean(L, 0); return 1; }
	bool ok = obj->setProperty(prop, val);
	lua_pushboolean(L, ok ? 1 : 0);
	return 1;
}

int ModApiUnapi::l_object_invoke_method(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	const char *method = luaL_checkstring(L, 2);
	if (!obj) { lua_pushnil(L); return 1; }

	unapi::ValueArray args;
	int top = lua_gettop(L);
	for (int i = 3; i <= top; i++) {
		args.push_back(luaToValue(L, i));
	}
	unapi::Value res = obj->invokeMethod(method, args);
	pushValueToLua(L, res);
	return 1;
}

int ModApiUnapi::l_object_get_child(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	const char *key = luaL_checkstring(L, 2);
	if (!obj) { lua_pushnil(L); return 1; }
	unapi::ObjectRefPtr child = obj->getChild(key);
	pushValueToLua(L, unapi::Value(child));
	return 1;
}

int ModApiUnapi::l_object_get_handle(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	if (!obj) { lua_pushinteger(L, 0); return 1; }
	lua_pushinteger(L, obj->getHandleId());
	return 1;
}

int ModApiUnapi::l_object_get_info(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	if (!obj) { lua_pushnil(L); return 1; }
	unapi::ValueMap info = unapi::UnapiRegistry::get().inspectObject(obj);
	pushValueToLua(L, unapi::Value(info));
	return 1;
}

int ModApiUnapi::l_object_index(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	const char *key = luaL_checkstring(L, 2);
	if (!obj) { lua_pushnil(L); return 1; }

	std::string skey = key;
	if (skey == "get_property") { lua_pushcfunction(L, l_object_get_property); return 1; }
	if (skey == "set_property") { lua_pushcfunction(L, l_object_set_property); return 1; }
	if (skey == "invoke_method") { lua_pushcfunction(L, l_object_invoke_method); return 1; }
	if (skey == "get_child") { lua_pushcfunction(L, l_object_get_child); return 1; }
	if (skey == "get_handle") { lua_pushcfunction(L, l_object_get_handle); return 1; }
	if (skey == "get_info") { lua_pushcfunction(L, l_object_get_info); return 1; }

	// Method lookup fallback
	lua_pushstring(L, skey.c_str());
	lua_pushcclosure(L, [](lua_State *L2) -> int {
		unapi::ObjectRefPtr o = checkUnapiObject(L2, 1);
		const char *mname = lua_tostring(L2, lua_upvalueindex(1));
		if (!o) { lua_pushnil(L2); return 1; }
		unapi::ValueArray args;
		int top = lua_gettop(L2);
		for (int i = 2; i <= top; i++) {
			args.push_back(luaToValue(L2, i));
		}
		unapi::Value r = o->invokeMethod(mname, args);
		pushValueToLua(L2, r);
		return 1;
	}, 1);

	// Test if property or child exists; if property exists, prefer returning property value
	unapi::Value prop = obj->getProperty(skey);
	if (!prop.isNil()) {
		lua_pop(L, 1); // pop closure
		pushValueToLua(L, prop);
		return 1;
	}

	unapi::ObjectRefPtr child = obj->getChild(skey);
	if (child) {
		lua_pop(L, 1); // pop closure
		pushValueToLua(L, unapi::Value(child));
		return 1;
	}

	return 1;
}

int ModApiUnapi::l_object_newindex(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	const char *key = luaL_checkstring(L, 2);
	unapi::Value val = luaToValue(L, 3);
	if (!obj) return 0;
	obj->setProperty(key, val);
	return 0;
}

int ModApiUnapi::l_object_eq(lua_State *L) {
	unapi::ObjectRefPtr obj1 = checkUnapiObject(L, 1);
	unapi::ObjectRefPtr obj2 = checkUnapiObject(L, 2);
	bool eq = (obj1 && obj2 && obj1 == obj2);
	lua_pushboolean(L, eq ? 1 : 0);
	return 1;
}

int ModApiUnapi::l_object_tostring(lua_State *L) {
	unapi::ObjectRefPtr obj = checkUnapiObject(L, 1);
	if (!obj) {
		lua_pushstring(L, "UnapiObject(null)");
		return 1;
	}
	std::string s = "UnapiObject(" + obj->getTypeName() + ":" + std::to_string(obj->getHandleId()) + ")";
	lua_pushstring(L, s.c_str());
	return 1;
}

int ModApiUnapi::l_object_gc(lua_State *L) {
	ObjectUserData *ud = (ObjectUserData *)luaL_checkudata(L, 1, UNAPI_OBJECT_METATABLE);
	if (ud) {
		ud->obj.reset();
	}
	return 0;
}

void ModApiUnapi::Initialize(lua_State *L, int top) {
	// Register foundational engine object types
	unapi::UnapiRegistry::get().initializeFoundations();
	unapi::registerAllFoundationTypes();

	// Create Metatable for unapi.object
	luaL_newmetatable(L, UNAPI_OBJECT_METATABLE);
	lua_pushcfunction(L, l_object_index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_object_newindex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, l_object_eq);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, l_object_tostring);
	lua_setfield(L, -2, "__tostring");
	lua_pushcfunction(L, l_object_gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);

	// Create 'unapi' global table
	lua_newtable(L);

	lua_pushcfunction(L, l_inspect);
	lua_setfield(L, -2, "inspect");

	lua_pushcfunction(L, l_create);
	lua_setfield(L, -2, "create");

	lua_pushcfunction(L, l_resolve);
	lua_setfield(L, -2, "resolve");

	lua_pushcfunction(L, l_hook);
	lua_setfield(L, -2, "hook");

	lua_pushcfunction(L, l_unhook);
	lua_setfield(L, -2, "unhook");

	lua_pushcfunction(L, l_list_types);
	lua_setfield(L, -2, "list_types");

	lua_pushcfunction(L, l_list_hooks);
	lua_setfield(L, -2, "list_hooks");

	// unapi.storage
	lua_newtable(L);
	lua_pushcfunction(L, l_storage_get);
	lua_setfield(L, -2, "get");
	lua_setfield(L, -2, "storage");

	// unapi.network
	lua_newtable(L);
	lua_pushcfunction(L, l_network_http_fetch);
	lua_setfield(L, -2, "http_fetch");
	lua_setfield(L, -2, "network");

	// Set 'unapi' global in Lua
	lua_pushvalue(L, -1);
	lua_setglobal(L, "unapi");

	// Also set in 'core.unapi' for compatibility
	lua_getglobal(L, "core");
	if (lua_istable(L, -1)) {
		lua_pushvalue(L, -2);
		lua_setfield(L, -2, "unapi");
	}
	lua_pop(L, 1);

	// Pop table from stack
	lua_pop(L, 1);

	infostream << "[UNAPI] Initialized Universal Engine Extension API in Lua" << std::endl;
}
