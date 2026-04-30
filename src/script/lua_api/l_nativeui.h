// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#pragma once

#include "l_base.h"

class ModApiNativeUI : public ModApiBase {
public:
	static void Initialize(lua_State *L, int top);

private:
	// Load layouts
	static int l_load_json(lua_State *L);
	static int l_load_layout(lua_State *L);

	// Lifecycle
	static int l_create(lua_State *L);
	static int l_show(lua_State *L);
	static int l_hide(lua_State *L);
	static int l_destroy(lua_State *L);

	// Widget access & updates
	static int l_get_widget(lua_State *L);
	static int l_set_text(lua_State *L);
	static int l_set_image(lua_State *L);
	static int l_set_style(lua_State *L);

	// Events
	static int l_on_event(lua_State *L);

	// Animations & Data binding (future)
	static int l_animate(lua_State *L);
	static int l_bind(lua_State *L);
	static int l_set_theme(lua_State *L);
};
