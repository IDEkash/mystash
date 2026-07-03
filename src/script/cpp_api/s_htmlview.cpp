// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "cpp_api/s_htmlview.h"

#include "cpp_api/s_internal.h"

#include "util/base64.h"

#include <json/json.h>
#include <memory>

#include "common/c_content.h"
#include "convert_json.h"
#ifdef __ANDROID__
#include "htmlview_jni.h"
#endif

static constexpr const char *HTMLVIEW_CALLBACKS_RKEY = "HTMLVIEW_CALLBACKS";
static constexpr const char *HTMLVIEW_JSON_CALLBACKS_RKEY = "HTMLVIEW_JSON_CALLBACKS";
static constexpr const char *HTMLVIEW_COMMANDS_RKEY = "HTMLVIEW_COMMANDS";
static constexpr const char *HTMLVIEW_PERMS_RKEY = "HTMLVIEW_PERMS";
static constexpr const char *HTMLVIEW_CAPTURE_CALLBACKS_RKEY = "HTMLVIEW_CAPTURE_CALLBACKS";
static constexpr const char *HTMLVIEW_READY_CALLBACKS_RKEY = "HTMLVIEW_READY_CALLBACKS";

constexpr static u16 HTMLVIEW_MAX_JSON_DEPTH = 1024;

void ScriptApiHTMLView::on_htmlview_message(const std::string &id, const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

	bool called = false;

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CALLBACKS_RKEY);
	if (lua_istable(L, -1)) {
		lua_pushlstring(L, id.c_str(), id.size());
		lua_gettable(L, -2);
		if (lua_isfunction(L, -1)) {
			lua_pushlstring(L, message.c_str(), message.size());
			PCALL_RES(lua_pcall(L, 1, 0, error_handler));
			called = true;
		} else {
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

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

			lua_pushnil(L);
			int nullindex = lua_gettop(L);
			if (ok) {
				if (!push_json_value(L, root, nullindex)) {
					errmsg = "depth exceeds lua stack limit";
					ok = false;
				}
			}
			if (!ok)
				lua_pushnil(L);
			lua_remove(L, nullindex);

			lua_pushlstring(L, message.c_str(), message.size());
			if (!ok)
				lua_pushlstring(L, errmsg.c_str(), errmsg.size());
			int nargs = ok ? 2 : 3;
			PCALL_RES(lua_pcall(L, nargs, 0, error_handler));
			called = true;
		} else {
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	if (!called) {
		lua_remove(L, error_handler);
		return;
	}
	lua_remove(L, error_handler);
}

void ScriptApiHTMLView::on_htmlview_command(const std::string &id, const std::string &callId,
		const std::string &cmd, const std::string &paramsJson)
{
	SCRIPTAPI_PRECHECKHEADER
#ifdef __ANDROID__
	int error_handler = PUSH_ERROR_HANDLER(L);

	auto send_response = [&](bool success, const std::string &resultJson, const std::string &error) {
		htmlview_jni_command_response(id, callId, success, resultJson, error);
	};

	// Check permissions
	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_PERMS_RKEY);
	if (lua_istable(L, -1)) {
		lua_pushlstring(L, id.c_str(), id.size());
		lua_gettable(L, -2);
		if (lua_istable(L, -1)) {
			lua_pushlstring(L, cmd.c_str(), cmd.size());
			lua_gettable(L, -2);
			bool allowed = lua_toboolean(L, -1);
			lua_pop(L, 1);
			if (!allowed) {
				send_response(false, "", "Permission denied for command: " + cmd);
				lua_pop(L, 1); // perms_for_id
				lua_pop(L, 1); // perms_table
				lua_remove(L, error_handler);
				return;
			}
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// Try to find registered custom command
	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_COMMANDS_RKEY);
	if (lua_istable(L, -1)) {
		lua_pushlstring(L, id.c_str(), id.size());
		lua_gettable(L, -2);
		if (lua_istable(L, -1)) {
			lua_pushlstring(L, cmd.c_str(), cmd.size());
			lua_gettable(L, -2);
			if (lua_isfunction(L, -1)) {
				// Params
				Json::Value root;
				bool ok = false;
				if (!paramsJson.empty()) {
					Json::CharReaderBuilder builder;
					const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
					std::string errmsg;
					ok = reader->parse(paramsJson.data(), paramsJson.data() + paramsJson.size(), &root, &errmsg);
				}

				lua_pushnil(L);
				int nullindex = lua_gettop(L);
				if (ok) push_json_value(L, root, nullindex);
				else lua_pushnil(L);
				lua_remove(L, nullindex);

				if (lua_pcall(L, 1, 1, error_handler) != 0) {
					std::string err = lua_tostring(L, -1);
					send_response(false, "", err);
					lua_pop(L, 1); // err
					lua_pop(L, 1); // cmds_for_id
					lua_pop(L, 1); // commands_table
					lua_remove(L, error_handler);
					return;
				}

				// Result
				if (lua_isnil(L, -1)) {
					send_response(true, "null", "");
				} else {
					Json::Value res;
					try {
						read_json_value(L, res, -1, HTMLVIEW_MAX_JSON_DEPTH);
						send_response(true, fastWriteJson(res), "");
					} catch (SerializationError &e) {
						send_response(false, "", std::string("Serialization error: ") + e.what());
					}
				}
				lua_pop(L, 1); // res
				lua_pop(L, 1); // cmds_for_id
				lua_pop(L, 1); // commands_table
				lua_remove(L, error_handler);
				return;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// Fallback: system commands (whitelist them)
	send_response(false, "", "Unknown or unauthorized command: " + cmd);

	lua_remove(L, error_handler);
#endif
}

void ScriptApiHTMLView::on_htmlview_capture(const std::string &id, const std::string &png_base64)
{
	SCRIPTAPI_PRECHECKHEADER

	if (!base64_is_valid(png_base64))
		return;
	std::string png = base64_decode(png_base64);

	int error_handler = PUSH_ERROR_HANDLER(L);

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_CAPTURE_CALLBACKS_RKEY);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_remove(L, error_handler);
		return;
	}

	lua_pushlstring(L, id.c_str(), id.size());
	lua_gettable(L, -2);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		lua_remove(L, error_handler);
		return;
	}

	lua_pushlstring(L, png.data(), png.size());
	PCALL_RES(lua_pcall(L, 1, 0, error_handler));

	lua_pop(L, 1); // callback table
	lua_remove(L, error_handler);
}

void ScriptApiHTMLView::on_htmlview_ready(const std::string &id)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

	lua_getfield(L, LUA_REGISTRYINDEX, HTMLVIEW_READY_CALLBACKS_RKEY);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_remove(L, error_handler);
		return;
	}

	lua_pushlstring(L, id.c_str(), id.size());
	lua_gettable(L, -2);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		lua_remove(L, error_handler);
		return;
	}

	PCALL_RES(lua_pcall(L, 0, 0, error_handler));

	lua_pop(L, 1); // callback table
	lua_remove(L, error_handler);
}
