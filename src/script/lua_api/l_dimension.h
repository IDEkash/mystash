#pragma once

#include "lua_api/l_base.h"

class ModApiDimension : public ModApiBase {
private:
    static int l_create_world(lua_State *L);
    static int l_delete_world(lua_State *L);
    static int l_link_mod(lua_State *L);
    static int l_enter_world(lua_State *L);

public:
    static void Initialize(lua_State *L, int top);
};
