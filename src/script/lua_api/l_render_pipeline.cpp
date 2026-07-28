#include "l_render_pipeline.h"
#include "client/render_camera.h"
#include "client/client.h"
#include "client/texturesource.h"
#include "client/camera.h"
#include "l_internal.h"
#include "script/common/c_converter.h"
#include <ISceneManager.h>
#include <ICameraSceneNode.h>
#include <ISceneNode.h>

// LuaRenderTarget static instances
std::set<LuaRenderTarget*> LuaRenderTarget::s_instances;

void LuaRenderTarget::clearAll()
{
    for (auto inst : s_instances) {
        inst->m_target = nullptr;
    }
    s_instances.clear();
}

LuaRenderTarget::LuaRenderTarget(GPURenderTarget *target) : m_target(target)
{
    s_instances.insert(this);
}

LuaRenderTarget::~LuaRenderTarget()
{
    s_instances.erase(this);
}

const char LuaRenderTarget::className[] = "RenderTarget";

const luaL_Reg LuaRenderTarget::methods[] = {
    luamethod(LuaRenderTarget, get_name),
    luamethod(LuaRenderTarget, get_width),
    luamethod(LuaRenderTarget, get_height),
    {0, 0}
};

void LuaRenderTarget::Register(lua_State *L)
{
    static const luaL_Reg metamethods[] = {
        {"__gc", gc_object},
        {"__tostring", mt_tostring},
        {0, 0}
    };
    registerClass<LuaRenderTarget>(L, methods, metamethods);
}

void LuaRenderTarget::create(lua_State *L, GPURenderTarget *target)
{
    LuaRenderTarget *o = new LuaRenderTarget(target);
    *(void **)(lua_newuserdata(L, sizeof(void *))) = o;
    luaL_getmetatable(L, className);
    lua_setmetatable(L, -2);
}

GPURenderTarget* LuaRenderTarget::getobject(lua_State *L, int narg)
{
    LuaRenderTarget *ref = checkObject<LuaRenderTarget>(L, narg);
    assert(ref);
    return ref->m_target;
}

int LuaRenderTarget::gc_object(lua_State *L)
{
    LuaRenderTarget *o = *(LuaRenderTarget **)(lua_touserdata(L, 1));
    if (o->m_target) {
        RenderCameraManager::get()->unregisterRenderTarget(o->m_target);
        delete o->m_target;
        o->m_target = nullptr;
    }
    delete o;
    return 0;
}

int LuaRenderTarget::mt_tostring(lua_State *L)
{
    LuaRenderTarget *o = *(LuaRenderTarget **)(lua_touserdata(L, 1));
    if (o && o->m_target) {
        lua_pushstring(L, o->m_target->getName().c_str());
        return 1;
    }
    lua_pushstring(L, "RenderTarget(nil)");
    return 1;
}

int LuaRenderTarget::l_get_name(lua_State *L)
{
    GPURenderTarget *target = getobject(L, 1);
    if (!target) {
        lua_pushstring(L, "RenderTarget(nil)");
        return 1;
    }
    lua_pushstring(L, target->getName().c_str());
    return 1;
}

int LuaRenderTarget::l_get_width(lua_State *L)
{
    GPURenderTarget *target = getobject(L, 1);
    if (!target) return 0;
    lua_pushinteger(L, target->getWidth());
    return 1;
}

int LuaRenderTarget::l_get_height(lua_State *L)
{
    GPURenderTarget *target = getobject(L, 1);
    if (!target) return 0;
    lua_pushinteger(L, target->getHeight());
    return 1;
}


// LuaRenderCamera static instances
std::set<LuaRenderCamera*> LuaRenderCamera::s_instances;

void LuaRenderCamera::clearAll()
{
    for (auto inst : s_instances) {
        inst->m_camera = nullptr;
    }
    s_instances.clear();
}

LuaRenderCamera::LuaRenderCamera(RenderCamera *camera) : m_camera(camera)
{
    s_instances.insert(this);
}

LuaRenderCamera::~LuaRenderCamera()
{
    s_instances.erase(this);
}

const char LuaRenderCamera::className[] = "RenderCamera";

const luaL_Reg LuaRenderCamera::methods[] = {
    luamethod(LuaRenderCamera, set_pos),
    luamethod(LuaRenderCamera, get_pos),
    luamethod(LuaRenderCamera, set_rotation),
    luamethod(LuaRenderCamera, get_rotation),
    luamethod(LuaRenderCamera, set_fov),
    luamethod(LuaRenderCamera, get_fov),
    luamethod(LuaRenderCamera, set_projection),
    luamethod(LuaRenderCamera, get_projection),
    luamethod(LuaRenderCamera, set_near_far),
    luamethod(LuaRenderCamera, get_near_far),
    luamethod(LuaRenderCamera, set_viewport),
    luamethod(LuaRenderCamera, get_viewport),
    luamethod(LuaRenderCamera, set_render_priority),
    luamethod(LuaRenderCamera, get_render_priority),
    luamethod(LuaRenderCamera, set_render_target),
    luamethod(LuaRenderCamera, get_render_target),
    luamethod(LuaRenderCamera, set_enabled),
    luamethod(LuaRenderCamera, get_enabled),
    luamethod(LuaRenderCamera, set_update_frequency),
    luamethod(LuaRenderCamera, get_update_frequency),
    luamethod(LuaRenderCamera, set_render_mask),
    luamethod(LuaRenderCamera, get_render_mask),
    luamethod(LuaRenderCamera, set_resolution_scaling),
    luamethod(LuaRenderCamera, get_resolution_scaling),
    luamethod(LuaRenderCamera, set_parent),
    {0, 0}
};

void LuaRenderCamera::Register(lua_State *L)
{
    static const luaL_Reg metamethods[] = {
        {"__gc", gc_object},
        {0, 0}
    };
    registerClass<LuaRenderCamera>(L, methods, metamethods);
}

void LuaRenderCamera::create(lua_State *L, RenderCamera *camera)
{
    LuaRenderCamera *o = new LuaRenderCamera(camera);
    *(void **)(lua_newuserdata(L, sizeof(void *))) = o;
    luaL_getmetatable(L, className);
    lua_setmetatable(L, -2);
}

RenderCamera* LuaRenderCamera::getobject(lua_State *L, int narg)
{
    LuaRenderCamera *ref = checkObject<LuaRenderCamera>(L, narg);
    assert(ref);
    return ref->m_camera;
}

int LuaRenderCamera::gc_object(lua_State *L)
{
    LuaRenderCamera *o = *(LuaRenderCamera **)(lua_touserdata(L, 1));
    if (o->m_camera) {
        RenderCameraManager::get()->unregisterCamera(o->m_camera);
        delete o->m_camera;
        o->m_camera = nullptr;
    }
    delete o;
    return 0;
}

int LuaRenderCamera::l_set_pos(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;

    v3f pos;
    if (lua_istable(L, 2)) {
        pos = read_v3f(L, 2) * BS;
    } else {
        pos.X = luaL_checknumber(L, 2) * BS;
        pos.Y = luaL_checknumber(L, 3) * BS;
        pos.Z = luaL_checknumber(L, 4) * BS;
    }

    cam->setPos(pos);
    return 0;
}

int LuaRenderCamera::l_get_pos(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    push_v3f(L, cam->getPos() / BS);
    return 1;
}

int LuaRenderCamera::l_set_rotation(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;

    v3f rot;
    if (lua_istable(L, 2)) {
        rot = read_v3f(L, 2);
    } else {
        rot.X = luaL_checknumber(L, 2);
        rot.Y = luaL_checknumber(L, 3);
        rot.Z = luaL_checknumber(L, 4);
    }

    cam->setRotation(rot);
    return 0;
}

int LuaRenderCamera::l_get_rotation(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    push_v3f(L, cam->getRotation());
    return 1;
}

int LuaRenderCamera::l_set_fov(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    f32 fov = luaL_checknumber(L, 2);
    cam->setFov(fov);
    return 0;
}

int LuaRenderCamera::l_get_fov(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushnumber(L, cam->getFov());
    return 1;
}

int LuaRenderCamera::l_set_projection(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    std::string proj = luaL_checkstring(L, 2);
    cam->setProjection(proj);
    return 0;
}

int LuaRenderCamera::l_get_projection(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushstring(L, cam->getProjection().c_str());
    return 1;
}

int LuaRenderCamera::l_set_near_far(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    f32 near_plane = luaL_checknumber(L, 2);
    f32 far_plane = luaL_checknumber(L, 3);
    cam->setNearFar(near_plane, far_plane);
    return 0;
}

int LuaRenderCamera::l_get_near_far(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushnumber(L, cam->getNear());
    lua_pushnumber(L, cam->getFar());
    return 2;
}

int LuaRenderCamera::l_set_viewport(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;

    core::rect<f32> vp;
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "x");
        vp.UpperLeftCorner.X = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 0.0f;
        lua_pop(L, 1);

        lua_getfield(L, 2, "y");
        vp.UpperLeftCorner.Y = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 0.0f;
        lua_pop(L, 1);

        lua_getfield(L, 2, "w");
        vp.LowerRightCorner.X = vp.UpperLeftCorner.X + (lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 1.0f);
        lua_pop(L, 1);

        lua_getfield(L, 2, "h");
        vp.LowerRightCorner.Y = vp.UpperLeftCorner.Y + (lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 1.0f);
        lua_pop(L, 1);
    } else {
        vp.UpperLeftCorner.X = luaL_checknumber(L, 2);
        vp.UpperLeftCorner.Y = luaL_checknumber(L, 3);
        vp.LowerRightCorner.X = vp.UpperLeftCorner.X + luaL_checknumber(L, 4);
        vp.LowerRightCorner.Y = vp.UpperLeftCorner.Y + luaL_checknumber(L, 5);
    }

    cam->setViewport(vp);
    return 0;
}

int LuaRenderCamera::l_get_viewport(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    core::rect<f32> vp = cam->getViewport();
    lua_newtable(L);
    lua_pushnumber(L, vp.UpperLeftCorner.X);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, vp.UpperLeftCorner.Y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, vp.getWidth());
    lua_setfield(L, -2, "w");
    lua_pushnumber(L, vp.getHeight());
    lua_setfield(L, -2, "h");
    return 1;
}

int LuaRenderCamera::l_set_render_priority(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    s32 priority = luaL_checkinteger(L, 2);
    cam->setRenderPriority(priority);
    return 0;
}

int LuaRenderCamera::l_get_render_priority(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushinteger(L, cam->getRenderPriority());
    return 1;
}

int LuaRenderCamera::l_set_render_target(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;

    if (lua_isnil(L, 2)) {
        cam->setRenderTarget(nullptr);
    } else {
        GPURenderTarget *target = LuaRenderTarget::getobject(L, 2);
        cam->setRenderTarget(target);
    }
    return 0;
}

int LuaRenderCamera::l_get_render_target(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    GPURenderTarget *target = cam->getRenderTarget();
    if (!target) return 0;
    LuaRenderTarget::create(L, target);
    return 1;
}

int LuaRenderCamera::l_set_enabled(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    bool enabled = lua_toboolean(L, 2);
    cam->setEnabled(enabled);
    return 0;
}

int LuaRenderCamera::l_get_enabled(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushboolean(L, cam->getEnabled());
    return 1;
}

int LuaRenderCamera::l_set_update_frequency(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    f32 freq = luaL_checknumber(L, 2);
    cam->setUpdateFrequency(freq);
    return 0;
}

int LuaRenderCamera::l_get_update_frequency(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushnumber(L, cam->getUpdateFrequency());
    return 1;
}

int LuaRenderCamera::l_set_render_mask(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    u32 mask = (u32)luaL_checkinteger(L, 2);
    cam->setRenderMask(mask);
    return 0;
}

int LuaRenderCamera::l_get_render_mask(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushinteger(L, cam->getRenderMask());
    return 1;
}

int LuaRenderCamera::l_set_resolution_scaling(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    f32 scale = luaL_checknumber(L, 2);
    cam->setResolutionScaling(scale);
    return 0;
}

int LuaRenderCamera::l_get_resolution_scaling(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;
    lua_pushnumber(L, cam->getResolutionScaling());
    return 1;
}

int LuaRenderCamera::l_set_parent(lua_State *L)
{
    RenderCamera *cam = getobject(L, 1);
    if (!cam) return 0;

    if (lua_isnil(L, 2)) {
        cam->setParent(nullptr);
    } else if (lua_isnumber(L, 2)) {
        u16 id = (u16)lua_tointeger(L, 2);
        Client *client = getClient(L);
        if (client) {
            ClientActiveObject *obj = client->getEnv().getActiveObject(id);
            if (obj) {
                cam->setParent(obj->getSceneNode());
            }
        }
    } else {
        std::string parent_type = luaL_checkstring(L, 2);
        Client *client = getClient(L);
        if (client && client->getCamera()) {
            if (parent_type == "player") {
                cam->setParent(client->getCamera()->getPlayerNode());
            } else if (parent_type == "head") {
                cam->setParent(client->getCamera()->getHeadNode());
            } else if (parent_type == "camera") {
                cam->setParent(client->getCamera()->getCameraNode());
            } else if (parent_type == "root") {
                cam->setParent(nullptr);
            }
        }
    }
    return 0;
}


// ModApiRenderPipeline implementation

int ModApiRenderPipeline::l_create_camera(lua_State *L)
{
    RenderCamera *cam = new RenderCamera();
    RenderCameraManager::get()->registerCamera(cam);
    LuaRenderCamera::create(L, cam);
    return 1;
}

int ModApiRenderPipeline::l_create_render_target(lua_State *L)
{
    u32 width = luaL_checkinteger(L, 1);
    u32 height = luaL_checkinteger(L, 2);
    std::string format = "rgba8";
    if (lua_isstring(L, 3)) {
        format = lua_tostring(L, 3);
    }

    static u32 rt_counter = 0;
    rt_counter++;
    std::string name = "rt_tex_" + std::to_string(rt_counter);

    GPURenderTarget *target = new GPURenderTarget(width, height, name, format);
    RenderCameraManager::get()->registerRenderTarget(target);

    // Also register this texture inside TextureSource!
    Client *client = getClient(L);
    if (client) {
        IWritableTextureSource *tsrc = dynamic_cast<IWritableTextureSource*>(client->tsrc());
        if (tsrc) {
            tsrc->registerRenderTargetTexture(name, target->getTexture());
        }
    }

    LuaRenderTarget::create(L, target);
    return 1;
}

void ModApiRenderPipeline::Initialize(lua_State *L, int top)
{
    lua_getglobal(L, "core");
    lua_pushcfunction(L, l_create_camera);
    lua_setfield(L, -2, "create_camera");
    lua_pushcfunction(L, l_create_render_target);
    lua_setfield(L, -2, "create_render_target");
    lua_pop(L, 1);
}
