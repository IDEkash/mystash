#include "LuaSubEngine.h"
#include "SubEngine.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "log.h"
#include "script/common/c_internal.h"

int LuaSubEngine::m_update_callback_ref = LUA_NOREF;
int LuaSubEngine::m_logic_callback_ref = LUA_NOREF;

void LuaSubEngine::Initialize(lua_State *L, int top) {
    lua_newtable(L);
    int subengine_top = lua_gettop(L);

    API_FCT(init);
    API_FCT(hook);
    API_FCT(update);
    API_FCT(logic);
    API_FCT(shutdown);

    lua_setfield(L, top, "subengine");
}

int LuaSubEngine::l_init(lua_State *L) {
    std::string libPath = luaL_checkstring(L, 1);

    bool success = subengine::SubEngine::getInstance().init(libPath);

    if (lua_isfunction(L, 2)) {
        lua_pushvalue(L, 2);
        lua_pushboolean(L, success);
        if (lua_pcall(L, 1, 0, 0) != 0) {
            errorstream << "LuaSubEngine: Error in init callback: " << lua_tostring(L, -1) << std::endl;
        }
    }

    lua_pushboolean(L, success);
    return 1;
}

int LuaSubEngine::l_hook(lua_State *L) {
    std::string propertyName = luaL_checkstring(L, 1);

    void* ptr = subengine::SubEngine::getInstance().getHookedProperty(propertyName);
    if (!ptr) {
        lua_pushnil(L);
        return 1;
    }

    // Return a table that can read the property
    lua_newtable(L);

    lua_pushlightuserdata(L, ptr);
    lua_setfield(L, -2, "_ptr");

    // metatable for __index
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_getfield(L, 1, "_ptr");
        float* p = (float*)lua_touserdata(L, -1);
        std::string key = luaL_checkstring(L, 2);
        if (key == "x" || key == "1") lua_pushnumber(L, p[0]);
        else if (key == "y" || key == "2") lua_pushnumber(L, p[1]);
        else if (key == "z" || key == "3") lua_pushnumber(L, p[2]);
        else lua_pushnil(L);
        return 1;
    });
    lua_settable(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_getfield(L, 1, "_ptr");
        float* p = (float*)lua_touserdata(L, -1);
        std::string key = luaL_checkstring(L, 2);
        float val = luaL_checknumber(L, 3);
        if (key == "x" || key == "1") p[0] = val;
        else if (key == "y" || key == "2") p[1] = val;
        else if (key == "z" || key == "3") p[2] = val;
        return 0;
    });
    lua_settable(L, -3);

    lua_setmetatable(L, -2);

    return 1;
}

int LuaSubEngine::l_update(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        if (m_update_callback_ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, m_update_callback_ref);
        }
        lua_pushvalue(L, 1);
        m_update_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}

int LuaSubEngine::l_logic(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        if (m_logic_callback_ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, m_logic_callback_ref);
        }
        lua_pushvalue(L, 1);
        m_logic_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}

int LuaSubEngine::l_shutdown(lua_State *L) {
    subengine::SubEngine::getInstance().shutdown();

    if (m_update_callback_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, m_update_callback_ref);
        m_update_callback_ref = LUA_NOREF;
    }
    if (m_logic_callback_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, m_logic_callback_ref);
        m_logic_callback_ref = LUA_NOREF;
    }

    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        if (lua_pcall(L, 0, 0, 0) != 0) {
            errorstream << "LuaSubEngine: Error in shutdown callback: " << lua_tostring(L, -1) << std::endl;
        }
    }
    return 0;
}

// We also need a way to call these callbacks from the game loop.
// Adding helper functions for that.

namespace subengine {
    void run_lua_update(lua_State *L) {
        if (LuaSubEngine::m_update_callback_ref != LUA_NOREF) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, LuaSubEngine::m_update_callback_ref);
            if (lua_pcall(L, 0, 0, 0) != 0) {
                errorstream << "LuaSubEngine: Error in update callback: " << lua_tostring(L, -1) << std::endl;
            }
        }
    }

    void run_lua_logic(lua_State *L) {
        if (LuaSubEngine::m_logic_callback_ref != LUA_NOREF) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, LuaSubEngine::m_logic_callback_ref);
            if (lua_pcall(L, 0, 0, 0) != 0) {
                errorstream << "LuaSubEngine: Error in logic callback: " << lua_tostring(L, -1) << std::endl;
            }
        }
    }
}
