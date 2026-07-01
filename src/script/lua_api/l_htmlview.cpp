// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_htmlview.h"

#include "common/c_converter.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_object.h"
#include "server/serveractiveobject.h"
#include "cpp_api/s_security.h"

#include <memory>


	#ifdef __ANDROID__
	#include "htmlview_jni.h"
	#include <cctype>
	#include <limits>
		#include <json/json.h>
		#include "convert_json.h"
		#include "common/c_content.h"
#include "constants.h"
		#endif

static constexpr const char *HTMLVIEW_CALLBACKS_RKEY = "HTMLVIEW_CALLBACKS";
static constexpr const char *HTMLVIEW_JSON_CALLBACKS_RKEY = "HTMLVIEW_JSON_CALLBACKS";
static constexpr const char *HTMLVIEW_CAPTURE_CALLBACKS_RKEY = "HTMLVIEW_CAPTURE_CALLBACKS";
static constexpr const char *HTMLVIEW_READY_CALLBACKS_RKEY = "HTMLVIEW_READY_CALLBACKS";

static constexpr u16 HTMLVIEW_MAX_JSON_DEPTH = 1024;

#ifdef __ANDROID__
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
#endif

int ModApiHTMLView::l_run(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string html = readParam<std::string>(L, 2);
	htmlview_jni_run(id, html);
#endif
	return 0;
}

int ModApiHTMLView::l_run_worker(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string html = readParam<std::string>(L, 2);
	htmlview_jni_run_worker(id, html);
#endif
	return 0;
}

int ModApiHTMLView::l_run_external(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string root_dir = readParam<std::string>(L, 2);
	std::string entry = "index.html";
	if (!lua_isnoneornil(L, 3))
		entry = readParam<std::string>(L, 3);

	CHECK_SECURE_PATH(L, root_dir.c_str(), false);
	htmlview_jni_run_external(id, root_dir, entry);
#endif
	return 0;
}

int ModApiHTMLView::l_run_external_worker(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string root_dir = readParam<std::string>(L, 2);
	std::string entry = "index.html";
	if (!lua_isnoneornil(L, 3))
		entry = readParam<std::string>(L, 3);

	CHECK_SECURE_PATH(L, root_dir.c_str(), false);
	htmlview_jni_run_external_worker(id, root_dir, entry);
#endif
	return 0;
}

int ModApiHTMLView::l_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	htmlview_jni_stop(id);
#endif
	return 0;
}

int ModApiHTMLView::l_display(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
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

	htmlview_jni_display(id, x, y, w, h, visible, fullscreen, safe_area,
		drag_embed, border_radius);
#endif
	return 0;
}

int ModApiHTMLView::l_send(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string message = readParam<std::string>(L, 2);
	htmlview_jni_send(id, message);
#endif
	return 0;
}

int ModApiHTMLView::l_send_json(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	Json::Value root;
	try {
		read_json_value(L, root, 2, HTMLVIEW_MAX_JSON_DEPTH);
	} catch (SerializationError &e) {
		return luaL_error(L, "htmlview.send_json: %s", e.what());
	}
	std::string out = fastWriteJson(root);
	htmlview_jni_send(id, out);
#endif
	return 0;
}

int ModApiHTMLView::l_navigate(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string url = readParam<std::string>(L, 2);
	htmlview_jni_navigate(id, url);
#endif
	return 0;
}

int ModApiHTMLView::l_inject(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string js = readParam<std::string>(L, 2);
	htmlview_jni_inject(id, js);
#endif
	return 0;
}

int ModApiHTMLView::l_pipe(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string from_id = readParam<std::string>(L, 1);
	std::string to_id = readParam<std::string>(L, 2);
	htmlview_jni_pipe(from_id, to_id);
#endif
	return 0;
}

int ModApiHTMLView::l_capture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
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

	htmlview_jni_capture(id, width, height);
#endif
	return 0;
}

int ModApiHTMLView::l_input(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	bool block_game_input = getboolfield_default(L, 2, "block_game_input", false);
	htmlview_jni_input(id, block_game_input);
#endif
	return 0;
}

int ModApiHTMLView::l_state(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);

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
	lua_pushnil(L);
	return 1;
#endif
}

int ModApiHTMLView::l_reload(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	htmlview_jni_reload(id);
#endif
	return 0;
}

int ModApiHTMLView::l_focus(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	htmlview_jni_focus(id);
#endif
	return 0;
}

int ModApiHTMLView::l_shared_set(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string key = readParam<std::string>(L, 1);
	const char *val = nullptr;
	if (!lua_isnil(L, 2))
		val = luaL_checkstring(L, 2);
	htmlview_jni_shared_set(key, val);
#endif
	return 0;
}

int ModApiHTMLView::l_shared_get(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string key = readParam<std::string>(L, 1);
	std::string val = htmlview_jni_shared_get(key);
	if (val.empty())
		lua_pushnil(L);
	else
		lua_pushlstring(L, val.c_str(), val.size());
	return 1;
#else
	lua_pushnil(L);
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

	v3f up = v3f(0, 1, 0);
	lua_getfield(L, 3, "up");
	if (!lua_isnil(L, -1)) up = read_v3f(L, -1);
	lua_pop(L, 1);

	float fov = getfloatfield_default(L, 3, "fov", 70.0f);
	float tilt = getfloatfield_default(L, 3, "tilt", 0.0f);
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

	bool smooth_pos = getboolfield_default(L, 3, "smooth_position", false);
	bool smooth_rot = getboolfield_default(L, 3, "smooth_rotation", false);
	float pos_smooth = getfloatfield_default(L, 3, "position_smoothing", 0.15f);
	float rot_smooth = getfloatfield_default(L, 3, "rotation_smoothing", 0.10f);
	std::string update_mode = getstringfield_default(L, 3, "update_mode", "continuous");

	htmlview_jni_set_viewport(id, name, pos, dir, up, fov, tilt, w, h, refresh_ms, format, quality,
			smooth_pos, smooth_rot, pos_smooth, rot_smooth, update_mode);
#endif
	return 0;
}

int ModApiHTMLView::l_get_viewport(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string name = readParam<std::string>(L, 2);

	v3f pos, dir, up;
	float fov, tilt, pos_smooth, rot_smooth;
	int w, h, quality;
	u32 refresh_ms;
	std::string format, update_mode;
	bool smooth_pos, smooth_rot;

	if (htmlview_jni_get_viewport(id, name, pos, dir, up, fov, tilt, w, h,
			refresh_ms, format, quality, smooth_pos, smooth_rot, pos_smooth, rot_smooth,
			update_mode)) {
		lua_newtable(L);
		push_v3f(L, pos / BS);
		lua_setfield(L, -2, "pos");
		push_v3f(L, dir);
		lua_setfield(L, -2, "dir");
		push_v3f(L, up);
		lua_setfield(L, -2, "up");
		lua_pushnumber(L, fov);
		lua_setfield(L, -2, "fov");
		lua_pushnumber(L, tilt);
		lua_setfield(L, -2, "tilt");
		lua_pushinteger(L, w);
		lua_setfield(L, -2, "width");
		lua_pushinteger(L, h);
		lua_setfield(L, -2, "height");
		lua_pushnumber(L, refresh_ms > 0 ? 1000.0 / refresh_ms : 0);
		lua_setfield(L, -2, "fps");
		lua_pushstring(L, format.c_str());
		lua_setfield(L, -2, "format");
		lua_pushinteger(L, quality);
		lua_setfield(L, -2, "quality");
		lua_pushboolean(L, smooth_pos);
		lua_setfield(L, -2, "smooth_position");
		lua_pushboolean(L, smooth_rot);
		lua_setfield(L, -2, "smooth_rotation");
		lua_pushnumber(L, pos_smooth);
		lua_setfield(L, -2, "position_smoothing");
		lua_pushnumber(L, rot_smooth);
		lua_setfield(L, -2, "rotation_smoothing");
		lua_pushstring(L, update_mode.c_str());
		lua_setfield(L, -2, "update_mode");
		return 1;
	}
#endif
	lua_pushnil(L);
	return 1;
}

int ModApiHTMLView::l_update_viewport(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string name = readParam<std::string>(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);

	v3f pos, dir, up;
	float fov, tilt, pos_smooth, rot_smooth;
	int w, h, quality;
	u32 refresh_ms;
	std::string format, update_mode;
	bool smooth_pos, smooth_rot;

	if (htmlview_jni_get_viewport(id, name, pos, dir, up, fov, tilt, w, h,
			refresh_ms, format, quality, smooth_pos, smooth_rot, pos_smooth, rot_smooth,
			update_mode)) {

		lua_getfield(L, 3, "pos");
		if (!lua_isnil(L, -1)) pos = read_v3f(L, -1) * BS;
		lua_pop(L, 1);

		lua_getfield(L, 3, "dir");
		if (!lua_isnil(L, -1)) dir = read_v3f(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 3, "up");
		if (!lua_isnil(L, -1)) up = read_v3f(L, -1);
		lua_pop(L, 1);

		fov = getfloatfield_default(L, 3, "fov", fov);
		tilt = getfloatfield_default(L, 3, "tilt", tilt);
		w = getintfield_default(L, 3, "width", w);
		h = getintfield_default(L, 3, "height", h);

		lua_getfield(L, 3, "fps");
		if (lua_isnumber(L, -1)) {
			float fps = lua_tonumber(L, -1);
			if (fps > 0) refresh_ms = 1000 / fps;
			else refresh_ms = 0;
		}
		lua_pop(L, 1);

		format = getstringfield_default(L, 3, "format", format.c_str());
		quality = getintfield_default(L, 3, "quality", quality);

		smooth_pos = getboolfield_default(L, 3, "smooth_position", smooth_pos);
		smooth_rot = getboolfield_default(L, 3, "smooth_rotation", smooth_rot);
		pos_smooth = getfloatfield_default(L, 3, "position_smoothing", pos_smooth);
		rot_smooth = getfloatfield_default(L, 3, "rotation_smoothing", rot_smooth);
		update_mode = getstringfield_default(L, 3, "update_mode", update_mode.c_str());

		htmlview_jni_set_viewport(id, name, pos, dir, up, fov, tilt, w, h, refresh_ms, format, quality,
				smooth_pos, smooth_rot, pos_smooth, rot_smooth, update_mode);
		lua_pushboolean(L, true);
		return 1;
	}
#endif
	lua_pushboolean(L, false);
	return 1;
}

int ModApiHTMLView::l_remove_viewport(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string name = readParam<std::string>(L, 2);
	htmlview_jni_remove_viewport(id, name);
#endif
	return 0;
}

int ModApiHTMLView::l_set_anchor(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	if (lua_isnoneornil(L, 2)) {
		htmlview_jni_set_anchor(id, "", v3f(0, 0, 0), 0, v3f(0, 0, 0), v2f(0, 0), v3f(0, 0, 0));
		return 0;
	}
	luaL_checktype(L, 2, LUA_TTABLE);

	std::string type = getstringfield_default(L, 2, "type", "2d");
	v3f pos = v3f(0, 0, 0);
	u16 object_id = 0;

	lua_getfield(L, 2, "target");
	if (lua_istable(L, -1)) {
		pos = read_v3f(L, -1) * BS;
	} else if (lua_isuserdata(L, -1)) {
		ObjectRef *ref = checkObject<ObjectRef>(L, -1);
		if (ref) {
			auto obj = ObjectRef::getobject(ref);
			if (obj)
				object_id = obj->getId();
		}
	}
	lua_pop(L, 1);

	v3f offset = v3f(0, 0, 0);
	lua_getfield(L, 2, "offset");
	if (!lua_isnil(L, -1))
		offset = read_v3f(L, -1) * BS;
	lua_pop(L, 1);

	v2f size = v2f(0, 0);
	lua_getfield(L, 2, "size");
	if (!lua_isnil(L, -1))
		size = read_v2f(L, -1);
	lua_pop(L, 1);

	v3f rotation = v3f(0, 0, 0);
	lua_getfield(L, 2, "rotation");
	if (!lua_isnil(L, -1))
		rotation = read_v3f(L, -1);
	lua_pop(L, 1);

	htmlview_jni_set_anchor(id, type, pos, object_id, offset, size, rotation);
#endif
	return 0;
}

int ModApiHTMLView::l_get_viewport_frame(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::string name = readParam<std::string>(L, 2);
	std::string frame = htmlview_jni_get_viewport_frame(id, name);
	if (!frame.empty()) {
		lua_pushlstring(L, frame.data(), frame.size());
		return 1;
	}
#endif
	lua_pushnil(L);
	return 1;
}

int ModApiHTMLView::l_get_viewport_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef __ANDROID__
	std::string id = readParam<std::string>(L, 1);
	std::vector<std::string> list = htmlview_jni_get_viewport_list(id);
	lua_newtable(L);
	for (size_t i = 0; i < list.size(); i++) {
		lua_pushlstring(L, list[i].data(), list[i].size());
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
#endif
	lua_newtable(L);
	return 1;
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

#ifndef __ANDROID__
static void log_htmlview_unavailable(lua_State *L)
{
	static bool warned = false;
	if (!warned) {
		warningstream << "htmlview API called on unsupported platform." << std::endl;
		warned = true;
	}
}
#endif

void ModApiHTMLView::Initialize(lua_State *L, int top)
{
		lua_newtable(L);
		int tbl = lua_gettop(L);

#ifndef __ANDROID__
		auto dummy = [](lua_State *L) -> int {
			log_htmlview_unavailable(L);
			return 0;
		};
		auto dummy_nil = [](lua_State *L) -> int {
			log_htmlview_unavailable(L);
			lua_pushnil(L);
			return 1;
		};

		registerFunction(L, "run", dummy, tbl);
		registerFunction(L, "run_worker", dummy, tbl);
		registerFunction(L, "run_external", dummy, tbl);
		registerFunction(L, "run_external_worker", dummy, tbl);
		registerFunction(L, "stop", dummy, tbl);
		registerFunction(L, "display", dummy, tbl);
		registerFunction(L, "send", dummy, tbl);
		registerFunction(L, "send_json", dummy, tbl);
		registerFunction(L, "navigate", dummy, tbl);
		registerFunction(L, "inject", dummy, tbl);
		registerFunction(L, "pipe", dummy, tbl);
		registerFunction(L, "capture", dummy, tbl);
		registerFunction(L, "input", dummy, tbl);
		registerFunction(L, "state", dummy_nil, tbl);
		registerFunction(L, "reload", dummy, tbl);
		registerFunction(L, "focus", dummy, tbl);
		registerFunction(L, "shared_set", dummy, tbl);
		registerFunction(L, "shared_get", dummy_nil, tbl);
		registerFunction(L, "on_message", l_on_message, tbl);
		registerFunction(L, "on_message_json", l_on_message_json, tbl);
		registerFunction(L, "on_capture", l_on_capture, tbl);
		registerFunction(L, "on_ready", l_on_ready, tbl);
		registerFunction(L, "set_viewport", dummy, tbl);
		registerFunction(L, "get_viewport", dummy_nil, tbl);
		registerFunction(L, "update_viewport", dummy_nil, tbl);
		registerFunction(L, "remove_viewport", dummy, tbl);
		registerFunction(L, "get_viewport_frame", dummy_nil, tbl);
		registerFunction(L, "get_viewport_list", dummy_nil, tbl);
		registerFunction(L, "set_anchor", dummy, tbl);
		registerFunction(L, "is_supported", l_is_supported, tbl);
#else
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
		registerFunction(L, "get_viewport", l_get_viewport, tbl);
		registerFunction(L, "update_viewport", l_update_viewport, tbl);
		registerFunction(L, "remove_viewport", l_remove_viewport, tbl);
		registerFunction(L, "get_viewport_frame", l_get_viewport_frame, tbl);
		registerFunction(L, "get_viewport_list", l_get_viewport_list, tbl);
		registerFunction(L, "set_anchor", l_set_anchor, tbl);
		registerFunction(L, "is_supported", l_is_supported, tbl);
#endif

	lua_pushvalue(L, tbl);
	lua_setglobal(L, "htmlview");
	lua_setfield(L, top, "htmlview");
}
