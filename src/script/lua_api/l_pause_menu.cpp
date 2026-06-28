// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 grorp

#include "l_pause_menu.h"
#include "client/keycode.h"
#include "gui/mainmenumanager.h"
#include "lua_api/l_internal.h"
#include "client/client.h"


int ModApiPauseMenu::l_show_touchscreen_layout(lua_State *L)
{
	g_gamecallback->touchscreenLayout();
	return 0;
}


int ModApiPauseMenu::l_is_internal_server(lua_State *L)
{
	lua_pushboolean(L, getClient(L)->m_internal_server);
	return 1;
}

int ModApiPauseMenu::l_set_paused(lua_State *L)
{
	bool paused = readParam<bool>(L, 1);
	getClient(L)->setPaused(paused);
	return 0;
}

int ModApiPauseMenu::l_get_paused(lua_State *L)
{
	lua_pushboolean(L, getClient(L)->isPaused());
	return 1;
}

void ModApiPauseMenu::Initialize(lua_State *L, int top)
{
	API_FCT(show_touchscreen_layout);
	API_FCT(is_internal_server);
	API_FCT(set_paused);
	API_FCT(get_paused);
}
