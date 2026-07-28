#pragma once

#include "l_base.h"
#include <string>
#include <set>

class RenderCamera;
class GPURenderTarget;

class LuaRenderTarget : public ModApiBase {
public:
    static const char className[];
    static void Register(lua_State *L);
    static void create(lua_State *L, GPURenderTarget *target);

    LuaRenderTarget(GPURenderTarget *target);
    ~LuaRenderTarget();

    static GPURenderTarget* getobject(lua_State *L, int narg);

    static std::set<LuaRenderTarget*> s_instances;
    static void clearAll();

private:
    GPURenderTarget *m_target = nullptr;

    static const luaL_Reg methods[];
    static int gc_object(lua_State *L);
    static int mt_tostring(lua_State *L);

    static int l_get_name(lua_State *L);
    static int l_get_width(lua_State *L);
    static int l_get_height(lua_State *L);
};

class LuaRenderCamera : public ModApiBase {
public:
    static const char className[];
    static void Register(lua_State *L);
    static void create(lua_State *L, RenderCamera *camera);

    LuaRenderCamera(RenderCamera *camera);
    ~LuaRenderCamera();

    static RenderCamera* getobject(lua_State *L, int narg);

    static std::set<LuaRenderCamera*> s_instances;
    static void clearAll();

private:
    RenderCamera *m_camera = nullptr;

    static const luaL_Reg methods[];
    static int gc_object(lua_State *L);

    static int l_set_pos(lua_State *L);
    static int l_get_pos(lua_State *L);
    static int l_set_rotation(lua_State *L);
    static int l_get_rotation(lua_State *L);
    static int l_set_fov(lua_State *L);
    static int l_get_fov(lua_State *L);
    static int l_set_projection(lua_State *L);
    static int l_get_projection(lua_State *L);
    static int l_set_near_far(lua_State *L);
    static int l_get_near_far(lua_State *L);
    static int l_set_viewport(lua_State *L);
    static int l_get_viewport(lua_State *L);
    static int l_set_render_priority(lua_State *L);
    static int l_get_render_priority(lua_State *L);
    static int l_set_render_target(lua_State *L);
    static int l_get_render_target(lua_State *L);
    static int l_set_enabled(lua_State *L);
    static int l_get_enabled(lua_State *L);
    static int l_set_update_frequency(lua_State *L);
    static int l_get_update_frequency(lua_State *L);
    static int l_set_render_mask(lua_State *L);
    static int l_get_render_mask(lua_State *L);
    static int l_set_resolution_scaling(lua_State *L);
    static int l_get_resolution_scaling(lua_State *L);
    static int l_set_parent(lua_State *L);
};

class ModApiRenderPipeline : public ModApiBase {
public:
    static void Initialize(lua_State *L, int top);

private:
    static int l_create_camera(lua_State *L);
    static int l_create_render_target(lua_State *L);
};
