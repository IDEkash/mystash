// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#include "l_nativeui.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "gui/guiNativeUI.h"
#include "gui/guiEngine.h"
#include "client/renderingengine.h"
#include "filesys.h"
#include <json/json.h>
#include "log.h"

int ModApiNativeUI::l_load_json(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string path = luaL_checkstring(L, 1);
	std::string content;
	if (!fs::ReadFile(path, content)) {
		lua_pushboolean(L, false);
		return 1;
	}

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if (!reader->parse(content.c_str(), content.c_str() + content.size(), &root, &errs)) {
		errorstream << "NativeUI: Failed to parse JSON: " << errs << std::endl;
		lua_pushboolean(L, false);
		return 1;
	}

	std::string name = fs::GetFilenameFromPath(path.c_str());
	engine->m_native_ui_layouts[name] = root;

	lua_pushboolean(L, true);
	return 1;
}

int ModApiNativeUI::l_load_layout(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string name = luaL_checkstring(L, 1);
	Json::Value root;
	read_json_value(L, root, 2, 10);
	engine->m_native_ui_layouts[name] = root;
	return 0;
}

int ModApiNativeUI::l_create(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string name = luaL_checkstring(L, 1);
	if (engine->m_native_ui_layouts.find(name) == engine->m_native_ui_layouts.end()) {
		lua_pushboolean(L, false);
		return 1;
	}

	GUINativeUI* ui = new GUINativeUI(
		engine->m_rendering_engine->get_gui_env(),
		engine->m_parent,
		-1,
		engine->m_menumanager,
		engine->m_texture_source.get(),
		engine->getScriptIface()
	);
	ui->loadLayout(engine->m_native_ui_layouts[name]);
	engine->m_native_uis[name] = ui;

	lua_pushboolean(L, true);
	return 1;
}

int ModApiNativeUI::l_show(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string name = luaL_checkstring(L, 1);
	auto it = engine->m_native_uis.find(name);
	if (it != engine->m_native_uis.end()) {
		it->second->setVisible(true);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

int ModApiNativeUI::l_hide(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string name = luaL_checkstring(L, 1);
	auto it = engine->m_native_uis.find(name);
	if (it != engine->m_native_uis.end()) {
		it->second->setVisible(false);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

int ModApiNativeUI::l_destroy(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string name = luaL_checkstring(L, 1);
	auto it = engine->m_native_uis.find(name);
	if (it != engine->m_native_uis.end()) {
		it->second->drop();
		engine->m_native_uis.erase(it);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

int ModApiNativeUI::l_set_text(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string id = luaL_checkstring(L, 1);
	std::wstring text = utf8_to_wide(luaL_checkstring(L, 2));

	for (auto const& [name, ui] : engine->m_native_uis) {
		gui::IGUIElement* widget = ui->getWidget(id);
		if (widget) {
			widget->setText(text.c_str());
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int ModApiNativeUI::l_set_image(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string id = luaL_checkstring(L, 1);
	std::string texture_name = luaL_checkstring(L, 2);

	for (auto const& [name, ui] : engine->m_native_uis) {
		gui::IGUIElement* widget = ui->getWidget(id);
		if (widget && widget->getType() == gui::EGUIET_IMAGE) {
			video::ITexture* tex = engine->m_texture_source->getTexture(texture_name);
			static_cast<gui::IGUIImage*>(widget)->setImage(tex);
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int ModApiNativeUI::l_get_widget(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string id = luaL_checkstring(L, 1);
	for (auto const& [name, ui] : engine->m_native_uis) {
		gui::IGUIElement* widget = ui->getWidget(id);
		if (widget) {
			lua_newtable(L);
			lua_pushstring(L, id.c_str());
			lua_setfield(L, -2, "id");
			// In a more complete implementation, we'd wrap the IGUIElement in a Lua object
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int ModApiNativeUI::l_set_style(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	if (!engine) return 0;

	std::string id = luaL_checkstring(L, 1);
	// Basic style support: color for panels
	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "color");
		if (lua_isstring(L, -1)) {
			std::string color_str = lua_tostring(L, -1);
			video::SColor color;
			if (parseColorString(color_str, color, true)) {
				for (auto const& [name, ui] : engine->m_native_uis) {
					gui::IGUIElement* widget = ui->getWidget(id);
					if (widget && widget->getType() == (gui::EGUI_ELEMENT_TYPE)GUI_ELEMENT_ID_BOX) {
						static_cast<GUIBox*>(widget)->setColor(color);
						lua_pushboolean(L, true);
						return 1;
					}
				}
			}
		}
		lua_pop(L, 1);
	}

	lua_pushboolean(L, false);
	return 1;
}

int ModApiNativeUI::l_on_event(lua_State *L)
{
	std::string widget_id = luaL_checkstring(L, 1);
	std::string event_type = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	// The logic to handle events is moved to a global core.native_ui_handler in Lua
	// for simpler integration with the current scripting architecture.

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "register_native_handler");
	if (lua_isfunction(L, -1)) {
		lua_pushstring(L, widget_id.c_str());
		lua_pushstring(L, event_type.c_str());
		lua_pushvalue(L, 3);
		lua_pcall(L, 3, 0, 0);
	}
	lua_pop(L, 1); // Pop core

	return 0;
}

int ModApiNativeUI::l_animate(lua_State *L)
{
	return 0;
}

int ModApiNativeUI::l_bind(lua_State *L)
{
	return 0;
}

int ModApiNativeUI::l_set_theme(lua_State *L)
{
	return 0;
}

void ModApiNativeUI::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int api_top = lua_gettop(L);

	API_FCT(load_json);
	API_FCT(load_layout);
	API_FCT(create);
	API_FCT(show);
	API_FCT(hide);
	API_FCT(destroy);
	API_FCT(get_widget);
	API_FCT(set_text);
	API_FCT(set_image);
	API_FCT(set_style);
	API_FCT(on_event);
	API_FCT(animate);
	API_FCT(bind);
	API_FCT(set_theme);

	lua_setfield(L, top, "ui");
}
