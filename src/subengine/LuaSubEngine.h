#pragma once

#include "script/lua_api/l_base.h"

class LuaSubEngine : public ModApiBase {
public:
    static void Initialize(lua_State *L, int top);

private:
    // init(libPath, callback)
    static int l_init(lua_State *L);
    // hook(propertyName, target)
    static int l_hook(lua_State *L);
    // update(callback)
    static int l_update(lua_State *L);
    // logic(callback)
    static int l_logic(lua_State *L);
    // shutdown(callback)
    static int l_shutdown(lua_State *L);

    static int m_update_callback_ref;
    static int m_logic_callback_ref;

    friend void run_lua_update(lua_State *L);
    friend void run_lua_logic(lua_State *L);
};

namespace subengine {
    void run_lua_update(lua_State *L);
    void run_lua_logic(lua_State *L);
}
