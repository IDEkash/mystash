// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "cpp_api/s_htmlview.h"

#include "cpp_api/s_internal.h"

#include "client/client.h"
#include "client/texturesource.h"
#include "client/renderingengine.h"
#include "util/base64.h"
#include <IFileSystem.h>
#include <IReadFile.h>

#include <json/json.h>
#include <memory>

#include "common/c_content.h"

static constexpr const char *HTMLVIEW_CALLBACKS_RKEY = "HTMLVIEW_CALLBACKS";
static constexpr const char *HTMLVIEW_JSON_CALLBACKS_RKEY = "HTMLVIEW_JSON_CALLBACKS";
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

void ScriptApiHTMLView::on_htmlview_capture(const std::string &id, const std::string &png_base64,
		const std::string &tex_name)
{
	SCRIPTAPI_PRECHECKHEADER

	if (!base64_is_valid(png_base64))
		return;
	std::string png = base64_decode(png_base64);

	if (!tex_name.empty()) {
		video::IVideoDriver *driver = RenderingEngine::get_video_driver();
		io::IFileSystem *fs = RenderingEngine::get_raw_device()->getFileSystem();

		io::IReadFile *file = fs->createMemoryReadFile(png.data(), png.size(), tex_name.c_str(), false);
		if (file) {
			video::IImage *img = driver->createImageFromFile(file);
			file->drop();
			if (img) {
				IWritableTextureSource *tsrc = (IWritableTextureSource *)getClient()->getTextureSource();
				tsrc->insertSourceImage(tex_name, img);
				img->drop();
			}
		}
	}

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
