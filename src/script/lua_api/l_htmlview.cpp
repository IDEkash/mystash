// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_htmlview.h"

#include "common/c_converter.h"
#include "lua_api/l_internal.h"
#include "cpp_api/s_security.h"

#include <memory>


#include <json/json.h>
#include "convert_json.h"
#include "common/c_content.h"
#include "constants.h"
#include <cctype>
#include <limits>

#ifdef __ANDROID__
#include "htmlview_jni.h"
#endif

static constexpr const char *HTMLVIEW_CALLBACKS_RKEY = "HTMLVIEW_CALLBACKS";
static constexpr const char *HTMLVIEW_JSON_CALLBACKS_RKEY = "HTMLVIEW_JSON_CALLBACKS";
static constexpr const char *HTMLVIEW_CAPTURE_CALLBACKS_RKEY = "HTMLVIEW_CAPTURE_CALLBACKS";
static constexpr const char *HTMLVIEW_READY_CALLBACKS_RKEY = "HTMLVIEW_READY_CALLBACKS";

static constexpr u16 HTMLVIEW_MAX_JSON_DEPTH = 1024;

static constexpr int CENTER_SENTINEL = std::numeric_limits<int>::min();

static bool isStringEqCI(lua_State *L, int idx, const char *s)
{
	if (!lua_isstring(L, idx))
		return false;
	const char *vv = lua_tostring(L, idx);
	if (!vv)
		return false;
	std::string v(vv);
	for (auto &c : v)
		c = (char)std::tolower((unsigned char)c);
	std::string ss(s);
	for (auto &c : ss)
		c = (char)std::tolower((unsigned char)c);
	return v == ss;
}

#ifndef __ANDROID__
#include <unordered_map>
#include <string>
#include <mutex>

struct MockView {
	std::string id;
	bool worker = false;
	bool visible = false;
	bool ready = false;
	std::string html;
	std::string root_dir;
	std::string entry;
};

static std::unordered_map<std::string, MockView> g_mock_views;
static std::unordered_map<std::string, std::string> g_mock_shared;
static std::unordered_map<std::string, std::string> g_mock_pipes;
static std::mutex g_mock_mutex;

static void mock_dispatch_message(lua_State *L, const std::string &id, const std::string &message)
{
	// 1. Check plain text callbacks
	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CALLBACKS_RKEY);
	if (lua_istable(L, -1)) {
		lua_pushlstring(L, id.c_str(), id.size());
		lua_gettable(L, -2);
		if (lua_isfunction(L, -1)) {
			lua_pushlstring(L, message.c_str(), message.size());
			if (lua_pcall(L, 1, 0, 0) != 0) {
				errorstream << "Error in htmlview mock message callback: "
							<< lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		} else {
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	// 2. Check JSON callbacks
	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_JSON_CALLBACKS_RKEY);
	if (lua_istable(L, -1)) {
		lua_pushlstring(L, id.c_str(), id.size());
		lua_gettable(L, -2);
		if (lua_isfunction(L, -1)) {
			Json::Value root;
			bool ok = false;
			std::string errmsg;
			{
				Json::CharReaderBuilder builder;
				builder.settings_["stackLimit"] = HTMLVIEW_MAX_JSON_DEPTH;
				builder.settings_["collectComments"] = false;
				const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
				ok = reader->parse(message.data(), message.data() + message.size(), &root, &errmsg);
			}

			if (ok) {
				lua_pushnil(L);
				int nullindex = lua_gettop(L);
				if (!push_json_value(L, root, nullindex)) {
					errmsg = "depth exceeds lua stack limit";
					ok = false;
				}
				lua_remove(L, nullindex);
			}

			if (!ok) {
				lua_pushnil(L);
			}

			lua_pushlstring(L, message.c_str(), message.size());
			if (!ok) {
				lua_pushlstring(L, errmsg.c_str(), errmsg.size());
			}

			int nargs = ok ? 2 : 3;
			if (lua_pcall(L, nargs, 0, 0) != 0) {
				errorstream << "Error in htmlview mock message JSON callback: "
							<< lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		} else {
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
}
#endif

int ModApiHTMLView::l_run(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string html = readParam<std::string>(L, 2);
#ifdef __ANDROID__
	htmlview_jni_run(id, html);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	MockView &v = g_mock_views[id];
	v.id = id;
	v.html = html;
	v.worker = false;
	v.visible = false;
	v.ready = true;
#endif
	return 0;
}

int ModApiHTMLView::l_run_worker(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string html = readParam<std::string>(L, 2);
#ifdef __ANDROID__
	htmlview_jni_run_worker(id, html);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	MockView &v = g_mock_views[id];
	v.id = id;
	v.html = html;
	v.worker = true;
	v.visible = false;
	v.ready = true;
#endif
	return 0;
}

int ModApiHTMLView::l_run_external(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string root_dir = readParam<std::string>(L, 2);
	std::string entry = "index.html";
	if (!lua_isnoneornil(L, 3))
		entry = readParam<std::string>(L, 3);

	CHECK_SECURE_PATH(L, root_dir.c_str(), false);
#ifdef __ANDROID__
	htmlview_jni_run_external(id, root_dir, entry);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	MockView &v = g_mock_views[id];
	v.id = id;
	v.root_dir = root_dir;
	v.entry = entry;
	v.worker = false;
	v.visible = false;
	v.ready = true;
#endif
	return 0;
}

int ModApiHTMLView::l_run_external_worker(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string root_dir = readParam<std::string>(L, 2);
	std::string entry = "index.html";
	if (!lua_isnoneornil(L, 3))
		entry = readParam<std::string>(L, 3);

	CHECK_SECURE_PATH(L, root_dir.c_str(), false);
#ifdef __ANDROID__
	htmlview_jni_run_external_worker(id, root_dir, entry);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	MockView &v = g_mock_views[id];
	v.id = id;
	v.root_dir = root_dir;
	v.entry = entry;
	v.worker = true;
	v.visible = false;
	v.ready = true;
#endif
	return 0;
}

int ModApiHTMLView::l_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
#ifdef __ANDROID__
	htmlview_jni_stop(id);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	g_mock_views.erase(id);
	g_mock_pipes.erase(id);
#endif
	return 0;
}

int ModApiHTMLView::l_display(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	bool visible = getboolfield_default(L, 2, "visible", true);
	bool safe_area = getboolfield_default(L, 2, "safe_area", true);
	bool fullscreen = getboolfield_default(L, 2, "fullscreen", false);
	bool drag_embed = getboolfield_default(L, 2, "drag_embed", false);
	if (!drag_embed)
		drag_embed = getboolfield_default(L, 2, "draggable", false);
	float border_radius = getfloatfield_default(L, 2, "border_radius", 0.0f);
	if (border_radius < 0.0f)
		border_radius = 0.0f;

	int x = 0;
	int y = 0;
	int w = 1;
	int h = 1;

	lua_getfield(L, 2, "x");
	if (lua_isnumber(L, -1))
		x = (int)lua_tointeger(L, -1);
	else if (isStringEqCI(L, -1, "center"))
		x = CENTER_SENTINEL;
	lua_pop(L, 1);

	lua_getfield(L, 2, "y");
	if (lua_isnumber(L, -1))
		y = (int)lua_tointeger(L, -1);
	else if (isStringEqCI(L, -1, "center"))
		y = CENTER_SENTINEL;
	lua_pop(L, 1);

	lua_getfield(L, 2, "width");
	if (lua_isnumber(L, -1))
		w = (int)lua_tointeger(L, -1);
	else if (isStringEqCI(L, -1, "fullscreen"))
		fullscreen = true;
	lua_pop(L, 1);

	lua_getfield(L, 2, "height");
	if (lua_isnumber(L, -1))
		h = (int)lua_tointeger(L, -1);
	else if (isStringEqCI(L, -1, "fullscreen"))
		fullscreen = true;
	lua_pop(L, 1);

#ifdef __ANDROID__
	htmlview_jni_display(id, x, y, w, h, visible, fullscreen, safe_area,
		drag_embed, border_radius);
#else
	(void)safe_area;
	(void)fullscreen;
	(void)x;
	(void)y;
	(void)w;
	(void)h;
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	auto it = g_mock_views.find(id);
	if (it != g_mock_views.end()) {
		it->second.visible = visible;
	}
#endif
	return 0;
}

int ModApiHTMLView::l_send(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string message = readParam<std::string>(L, 2);
#ifdef __ANDROID__
	htmlview_jni_send(id, message);
#else
	std::string target_id = id;
	{
		std::lock_guard<std::mutex> lock(g_mock_mutex);
		auto it = g_mock_pipes.find(id);
		if (it != g_mock_pipes.end()) {
			target_id = it->second;
		}
	}
	mock_dispatch_message(L, target_id, message);
#endif
	return 0;
}

int ModApiHTMLView::l_send_json(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	Json::Value root;
	try {
		read_json_value(L, root, 2, HTMLVIEW_MAX_JSON_DEPTH);
	} catch (SerializationError &e) {
		return luaL_error(L, "htmlview.send_json: %s", e.what());
	}
	std::string out = fastWriteJson(root);
#ifdef __ANDROID__
	htmlview_jni_send(id, out);
#else
	std::string target_id = id;
	{
		std::lock_guard<std::mutex> lock(g_mock_mutex);
		auto it = g_mock_pipes.find(id);
		if (it != g_mock_pipes.end()) {
			target_id = it->second;
		}
	}
	mock_dispatch_message(L, target_id, out);
#endif
	return 0;
}

int ModApiHTMLView::l_navigate(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string url = readParam<std::string>(L, 2);
#ifdef __ANDROID__
	htmlview_jni_navigate(id, url);
#endif
	return 0;
}

int ModApiHTMLView::l_inject(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	std::string js = readParam<std::string>(L, 2);
#ifdef __ANDROID__
	htmlview_jni_inject(id, js);
#endif
	return 0;
}

int ModApiHTMLView::l_pipe(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string from_id = readParam<std::string>(L, 1);
	std::string to_id = readParam<std::string>(L, 2);
#ifdef __ANDROID__
	htmlview_jni_pipe(from_id, to_id);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	g_mock_pipes[from_id] = to_id;
#endif
	return 0;
}

int ModApiHTMLView::l_capture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	int width = 0;
	int height = 0;
	if (lua_istable(L, 2)) {
		width = getintfield_default(L, 2, "width", 0);
		height = getintfield_default(L, 2, "height", 0);
	}
	if (width < 0)
		width = 0;
	if (height < 0)
		height = 0;

#ifdef __ANDROID__
	htmlview_jni_capture(id, width, height);
#endif
	return 0;
}

int ModApiHTMLView::l_input(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	bool block_game_input = getboolfield_default(L, 2, "block_game_input", false);
#ifdef __ANDROID__
	htmlview_jni_input(id, block_game_input);
#else
	(void)block_game_input;
#endif
	return 0;
}

int ModApiHTMLView::l_state(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);

#ifdef __ANDROID__
	std::string json = htmlview_jni_state(id);
	if (json.empty()) {
		lua_pushnil(L);
		return 1;
	}
	Json::Value root;
	{
		Json::CharReaderBuilder builder;
		builder.settings_["stackLimit"] = HTMLVIEW_MAX_JSON_DEPTH;
		builder.settings_["collectComments"] = false;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		std::string errmsg;
		if (!reader->parse(json.data(), json.data() + json.size(), &root, &errmsg)) {
			lua_pushnil(L);
			return 1;
		}
	}
	if (!push_json_value(L, root, 0)) {
		lua_pushnil(L);
		return 1;
	}
	return 1;
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	auto it = g_mock_views.find(id);
	if (it == g_mock_views.end()) {
		lua_pushnil(L);
	} else {
		lua_newtable(L);
		lua_pushboolean(L, true);
		lua_setfield(L, -2, "exists");
		lua_pushboolean(L, it->second.worker);
		lua_setfield(L, -2, "worker");
		lua_pushboolean(L, it->second.visible);
		lua_setfield(L, -2, "visible");
		lua_pushboolean(L, it->second.ready);
		lua_setfield(L, -2, "ready");
	}
	return 1;
#endif
}

int ModApiHTMLView::l_reload(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
#ifdef __ANDROID__
	htmlview_jni_reload(id);
#endif
	return 0;
}

int ModApiHTMLView::l_focus(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
#ifdef __ANDROID__
	htmlview_jni_focus(id);
#endif
	return 0;
}

int ModApiHTMLView::l_shared_set(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string key = readParam<std::string>(L, 1);
#ifdef __ANDROID__
	const char *val = nullptr;
	if (!lua_isnil(L, 2))
		val = luaL_checkstring(L, 2);
	htmlview_jni_shared_set(key, val);
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	if (lua_isnil(L, 2)) {
		g_mock_shared.erase(key);
	} else {
		g_mock_shared[key] = luaL_checkstring(L, 2);
	}
#endif
	return 0;
}

int ModApiHTMLView::l_shared_get(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string key = readParam<std::string>(L, 1);
#ifdef __ANDROID__
	std::string val = htmlview_jni_shared_get(key);
	if (val.empty())
		lua_pushnil(L);
	else
		lua_pushlstring(L, val.c_str(), val.size());
	return 1;
#else
	std::lock_guard<std::mutex> lock(g_mock_mutex);
	auto it = g_mock_shared.find(key);
	if (it == g_mock_shared.end()) {
		lua_pushnil(L);
	} else {
		lua_pushlstring(L, it->second.c_str(), it->second.size());
	}
	return 1;
#endif
}


int ModApiHTMLView::l_on_message(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	bool clear = lua_isnil(L, 2);
	if (!clear)
		luaL_checktype(L, 2, LUA_TFUNCTION);

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CALLBACKS_RKEY);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CALLBACKS_RKEY);
	}

	lua_pushlstring(L, id.c_str(), id.size());
	if (clear)
		lua_pushnil(L);
	else
		lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pop(L, 1);

#ifndef __ANDROID__
	// If mock view exists, trigger ready callback immediately!
	if (!clear) {
		std::lock_guard<std::mutex> lock(g_mock_mutex);
		if (g_mock_views.find(id) != g_mock_views.end()) {
			lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_READY_CALLBACKS_RKEY);
			if (lua_istable(L, -1)) {
				lua_pushlstring(L, id.c_str(), id.size());
				lua_gettable(L, -2);
				if (lua_isfunction(L, -1)) {
					if (lua_pcall(L, 0, 0, 0) != 0) {
						errorstream << "Error in htmlview mock ready callback: "
									<< lua_tostring(L, -1) << std::endl;
						lua_pop(L, 1);
					}
				} else {
					lua_pop(L, 1);
				}
			}
			lua_pop(L, 1);
		}
	}
#endif

	return 0;
}

int ModApiHTMLView::l_on_message_json(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	bool clear = lua_isnil(L, 2);
	if (!clear)
		luaL_checktype(L, 2, LUA_TFUNCTION);

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_JSON_CALLBACKS_RKEY);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, HTMLVIEW_JSON_CALLBACKS_RKEY);
	}

	lua_pushlstring(L, id.c_str(), id.size());
	if (clear)
		lua_pushnil(L);
	else
		lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pop(L, 1);
	return 0;
}

int ModApiHTMLView::l_on_capture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	bool clear = lua_isnil(L, 2);
	if (!clear)
		luaL_checktype(L, 2, LUA_TFUNCTION);

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CAPTURE_CALLBACKS_RKEY);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CAPTURE_CALLBACKS_RKEY);
	}

	lua_pushlstring(L, id.c_str(), id.size());
	if (clear)
		lua_pushnil(L);
	else
		lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pop(L, 1);
	return 0;
}

int ModApiHTMLView::l_on_ready(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string id = readParam<std::string>(L, 1);
	bool clear = lua_isnil(L, 2);
	if (!clear)
		luaL_checktype(L, 2, LUA_TFUNCTION);

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_READY_CALLBACKS_RKEY);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, HTMLVIEW_READY_CALLBACKS_RKEY);
	}

	lua_pushlstring(L, id.c_str(), id.size());
	if (clear)
		lua_pushnil(L);
	else
		lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pop(L, 1);
	return 0;
}

int ModApiHTMLView::l_set_viewport(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string name = readParam<std::string>(L, 2);
	if (lua_isnoneornil(L, 3)) {
		htmlview_jni_remove_viewport(id, name);
		return 0;
	}
	luaL_checktype(L, 3, LUA_TTABLE);
	v3f pos = v3f(0, 0, 0);
	lua_getfield(L, 3, "pos");
	if (!lua_isnil(L, -1)) pos = read_v3f(L, -1);
	lua_pop(L, 1);
	pos *= BS;

	v3f dir = v3f(0, 0, 1);
	lua_getfield(L, 3, "dir");
	if (!lua_isnil(L, -1)) dir = read_v3f(L, -1);
	lua_pop(L, 1);

	float fov = getfloatfield_default(L, 3, "fov", 70.0f);
	int w = getintfield_default(L, 3, "width", 256);
	int h = getintfield_default(L, 3, "height", 256);

	u32 refresh_ms = 50;
	lua_getfield(L, 3, "fps");
	if (lua_isnumber(L, -1)) {
		float fps = lua_tonumber(L, -1);
		if (fps > 0) refresh_ms = 1000 / fps;
		else refresh_ms = 0;
	}
	lua_pop(L, 1);

	std::string format = getstringfield_default(L, 3, "format", "jpeg");
	int quality = getintfield_default(L, 3, "quality", 70);

	htmlview_jni_set_viewport(id, name, pos, dir, fov, w, h, refresh_ms, format, quality);
#endif
	return 0;
}

int ModApiHTMLView::l_is_supported(lua_State *L)
{
#ifdef __ANDROID__
	lua_pushboolean(L, true);
#else
	lua_pushboolean(L, false);
#endif
	return 1;
}

void ModApiHTMLView::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int tbl = lua_gettop(L);

	registerFunction(L, "run", l_run, tbl);
	registerFunction(L, "run_worker", l_run_worker, tbl);
	registerFunction(L, "run_external", l_run_external, tbl);
	registerFunction(L, "run_external_worker", l_run_external_worker, tbl);
	registerFunction(L, "stop", l_stop, tbl);
	registerFunction(L, "display", l_display, tbl);
	registerFunction(L, "send", l_send, tbl);
	registerFunction(L, "send_json", l_send_json, tbl);
	registerFunction(L, "navigate", l_navigate, tbl);
	registerFunction(L, "inject", l_inject, tbl);
	registerFunction(L, "pipe", l_pipe, tbl);
	registerFunction(L, "capture", l_capture, tbl);
	registerFunction(L, "input", l_input, tbl);
	registerFunction(L, "state", l_state, tbl);
	registerFunction(L, "reload", l_reload, tbl);
	registerFunction(L, "focus", l_focus, tbl);
	registerFunction(L, "shared_set", l_shared_set, tbl);
	registerFunction(L, "shared_get", l_shared_get, tbl);
	registerFunction(L, "on_message", l_on_message, tbl);
	registerFunction(L, "on_message_json", l_on_message_json, tbl);
	registerFunction(L, "on_capture", l_on_capture, tbl);
	registerFunction(L, "on_ready", l_on_ready, tbl);
	registerFunction(L, "set_viewport", l_set_viewport, tbl);
	registerFunction(L, "is_supported", l_is_supported, tbl);

	lua_pushvalue(L, tbl);
	lua_setglobal(L, "htmlview");
	lua_setfield(L, top, "htmlview");
}
