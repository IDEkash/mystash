#include "render_camera.h"
#include "renderingengine.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "script/lua_api/l_render_pipeline.h"
#include <IrrlichtDevice.h>
#include <IVideoDriver.h>
#include <ISceneManager.h>
#include <ICameraSceneNode.h>
#include <ISceneNode.h>
#include <ITexture.h>
#include <algorithm>
#include <cmath>

static void applyRenderMask(scene::ISceneNode *node, u32 mask, std::vector<std::pair<scene::ISceneNode*, bool>> &saved_visibility)
{
    if (!node) return;
    s32 id = node->getID();
    // In Irrlicht/Luanti, nodes with ID -1 are default and are always visible.
    // Otherwise, check if ID matches the render mask.
    if (id != -1 && (id & mask) == 0) {
        if (node->isVisible()) {
            saved_visibility.push_back({node, true});
            node->setVisible(false);
        }
    }
    for (auto const &child : node->getChildren()) {
        applyRenderMask(child, mask, saved_visibility);
    }
}

// GPURenderTarget implementation

GPURenderTarget::GPURenderTarget(u32 width, u32 height, const std::string &name, const std::string &format_str)
    : m_name(name), m_width(width), m_height(height)
{
    video::IVideoDriver *driver = RenderingEngine::get_video_driver();
    video::ECOLOR_FORMAT format = video::ECF_A8R8G8B8;
    if (format_str == "rgb8") {
        format = video::ECF_R8G8B8;
    } else if (format_str == "rgb565") {
        format = video::ECF_R5G6B5;
    } else if (format_str == "rgba16f") {
        format = video::ECF_A16B16G16R16F;
    } else if (format_str == "rgba32f") {
        format = video::ECF_A32B32G32R32F;
    }

    m_texture = driver->addRenderTargetTexture(core::dimension2d<u32>(width, height), name.c_str(), format);
    if (m_texture) {
        m_texture->grab();
    }
}

GPURenderTarget::~GPURenderTarget()
{
    if (m_texture) {
        video::IVideoDriver *driver = RenderingEngine::get_video_driver();
        if (driver) {
            driver->removeTexture(m_texture);
        }
        m_texture->drop();
    }
}

// RenderCamera implementation

RenderCamera::RenderCamera()
{
    scene::ISceneManager *smgr = RenderingEngine::get_raw_device()->getSceneManager();
    m_cameranode = smgr->addCameraSceneNode(smgr->getRootSceneNode());
    if (m_cameranode) {
        m_cameranode->grab();
        m_cameranode->bindTargetAndRotation(true);
    }
}

RenderCamera::~RenderCamera()
{
    if (m_cameranode) {
        m_cameranode->remove();
        m_cameranode->drop();
    }
}

void RenderCamera::setPos(const v3f &pos)
{
    if (m_cameranode) {
        m_cameranode->setPosition(pos);
    }
}

v3f RenderCamera::getPos() const
{
    if (m_cameranode) {
        return m_cameranode->getPosition();
    }
    return v3f(0, 0, 0);
}

void RenderCamera::setRotation(const v3f &rot)
{
    if (m_cameranode) {
        m_cameranode->setRotation(rot);
    }
}

v3f RenderCamera::getRotation() const
{
    if (m_cameranode) {
        return m_cameranode->getRotation();
    }
    return v3f(0, 0, 0);
}

void RenderCamera::setFov(f32 fov)
{
    m_fov = fov;
    if (m_cameranode) {
        m_cameranode->setFOV(fov * core::DEGTORAD);
    }
}

void RenderCamera::setProjection(const std::string &proj)
{
    m_projection = proj;
}

void RenderCamera::setNearFar(f32 near_plane, f32 far_plane)
{
    m_near_plane = near_plane;
    m_far_plane = far_plane;
    if (m_cameranode) {
        m_cameranode->setNearValue(near_plane);
        m_cameranode->setFarValue(far_plane);
    }
}

void RenderCamera::setViewport(const core::rect<f32> &viewport)
{
    m_viewport = viewport;
}

bool RenderCamera::shouldRender(f32 dtime)
{
    if (m_update_frequency <= 0.0f) {
        return true;
    }
    m_time_since_last_render += dtime;
    if (m_time_since_last_render >= m_update_frequency) {
        m_time_since_last_render = std::fmod(m_time_since_last_render, m_update_frequency);
        return true;
    }
    return false;
}

void RenderCamera::setParent(scene::ISceneNode *parent)
{
    if (m_cameranode) {
        if (parent) {
            m_cameranode->setParent(parent);
        } else {
            scene::ISceneManager *smgr = RenderingEngine::get_raw_device()->getSceneManager();
            m_cameranode->setParent(smgr->getRootSceneNode());
        }
    }
}

scene::ISceneNode *RenderCamera::getParent() const
{
    if (m_cameranode) {
        return m_cameranode->getParent();
    }
    return nullptr;
}


// RenderCameraManager implementation

RenderCameraManager* RenderCameraManager::get()
{
    static RenderCameraManager instance;
    return &instance;
}

void RenderCameraManager::registerCamera(RenderCamera *camera)
{
    m_cameras.push_back(camera);
}

void RenderCameraManager::unregisterCamera(RenderCamera *camera)
{
    m_cameras.erase(std::remove(m_cameras.begin(), m_cameras.end(), camera), m_cameras.end());
}

void RenderCameraManager::registerRenderTarget(GPURenderTarget *target)
{
    m_render_targets.push_back(target);
}

void RenderCameraManager::unregisterRenderTarget(GPURenderTarget *target)
{
    m_render_targets.erase(std::remove(m_render_targets.begin(), m_render_targets.end(), target), m_render_targets.end());
}

void RenderCameraManager::clear()
{
    // Wipe weak-pointers in active Lua wrappers to prevent Use-After-Free crashes
    LuaRenderCamera::clearAll();
    LuaRenderTarget::clearAll();

    // Now delete C++ cameras and targets
    for (auto cam : m_cameras) {
        delete cam;
    }
    m_cameras.clear();

    for (auto target : m_render_targets) {
        delete target;
    }
    m_render_targets.clear();
}

void RenderCameraManager::renderAll(video::SColor clear_color, bool offscreen_only)
{
    video::IVideoDriver* driver = RenderingEngine::get_video_driver();
    IrrlichtDevice* device = RenderingEngine::get_raw_device();
    if (!driver || !device) return;

    // Measure dtime since last render pass
    static u32 last_time = 0;
    u32 cur_time = device->getTimer()->getTime();
    f32 dtime = 0.0f;
    if (last_time != 0 && cur_time > last_time) {
        dtime = (cur_time - last_time) / 1000.0f;
    }
    last_time = cur_time;

    std::vector<RenderCamera*> active_cameras;
    for (auto cam : m_cameras) {
        if (!cam->getEnabled() || !cam->shouldRender(dtime)) {
            continue;
        }

        bool is_offscreen = (cam->getRenderTarget() != nullptr);
        if (offscreen_only == is_offscreen) {
            active_cameras.push_back(cam);
        }
    }

    if (active_cameras.empty()) {
        return;
    }

    std::sort(active_cameras.begin(), active_cameras.end(), [](RenderCamera *a, RenderCamera *b) {
        return a->getRenderPriority() < b->getRenderPriority();
    });

    scene::ISceneManager* smgr = device->getSceneManager();

    // Save active state
    scene::ICameraSceneNode *old_camera = smgr->getActiveCamera();
    core::rect<s32> old_viewport = driver->getViewPort();

    // Save Map camera culling state
    ClientMap *client_map = m_client ? &(m_client->getEnv().getClientMap()) : nullptr;
    v3f old_map_pos, old_map_dir;
    f32 old_map_fov = 1.0f;
    v3s16 old_map_offset;
    video::SColor old_map_light;
    bool saved_map = false;

    if (client_map) {
        old_map_pos = client_map->getCameraPosition();
        old_map_dir = client_map->getCameraDirection();
        old_map_fov = client_map->getCameraFov();
        old_map_offset = client_map->getCameraOffset();
        old_map_light = client_map->getCameraLightColor();
        saved_map = true;
    }

    for (auto cam : active_cameras) {
        scene::ICameraSceneNode *cam_node = cam->getIrrlichtCameraNode();
        if (!cam_node) continue;

        // Apply near/far plane values
        cam_node->setNearValue(cam->getNear());
        cam_node->setFarValue(cam->getFar());

        // Viewport and aspect ratio setup
        core::rect<f32> norm_vp = cam->getViewport();
        GPURenderTarget *rt = cam->getRenderTarget();
        video::ITexture *rt_tex = rt ? rt->getTexture() : nullptr;

        core::dimension2du size = rt_tex ? rt_tex->getSize() : driver->getScreenSize();

        // Resolution scaling
        f32 scaling = cam->getResolutionScaling();
        s32 vp_w = (s32)(norm_vp.getWidth() * size.Width * scaling);
        s32 vp_h = (s32)(norm_vp.getHeight() * size.Height * scaling);
        s32 vp_x = (s32)(norm_vp.UpperLeftCorner.X * size.Width);
        s32 vp_y = (s32)(norm_vp.UpperLeftCorner.Y * size.Height);

        core::rect<s32> pixel_vp(vp_x, vp_y, vp_x + vp_w, vp_y + vp_h);

        f32 aspect = 1.0f;
        if (vp_h > 0) {
            aspect = (f32)vp_w / (f32)vp_h;
        }
        cam_node->setAspectRatio(aspect);

        // Apply projection mode
        f32 fov = cam->getFov() * core::DEGTORAD;
        if (cam->getProjection() == "orthographic") {
            f32 ortho_w = 20.0f;
            f32 ortho_h = ortho_w / aspect;
            core::matrix4 ortho_matrix;
            ortho_matrix.buildProjectionMatrixOrthoLH(ortho_w, ortho_h, cam->getNear(), cam->getFar());
            cam_node->setProjectionMatrix(ortho_matrix, true);
        } else {
            core::matrix4 perspective_matrix;
            perspective_matrix.buildProjectionMatrixPerspectiveFovLH(fov, aspect, cam->getNear(), cam->getFar());
            cam_node->setProjectionMatrix(perspective_matrix, false);
        }

        // Set active camera
        smgr->setActiveCamera(cam_node);

        // Set render target & clear viewport
        if (rt_tex) {
            driver->setRenderTarget(rt_tex, true, true, clear_color);
        } else {
            driver->setRenderTarget(nullptr, false, false);
            driver->setViewPort(pixel_vp);
            // Draw a colored 2D rectangle over the viewport area and clear depth
            driver->draw2DRectangle(clear_color, pixel_vp);
            driver->clearZBuffer();
        }

        driver->setViewPort(pixel_vp);

        // Update ClientMap's camera & culling draw list!
        if (client_map) {
            // Transform local/absolute coordinate representation
            v3f cam_pos = cam_node->getAbsolutePosition();
            v3f cam_dir = cam_node->getTarget() - cam_pos;
            cam_dir.normalize();

            // Re-center offset for culling
            v3s16 cam_offset = floatToInt(cam_pos, BS);
            client_map->updateCamera(cam_pos - intToFloat(cam_offset, BS), cam_dir, fov, cam_offset, old_map_light);
            client_map->updateDrawList();
        }

        // Apply render mask (visibility culling)
        std::vector<std::pair<scene::ISceneNode*, bool>> saved_visibility;
        u32 mask = cam->getRenderMask();
        if (mask != 0xFFFFFFFF) {
            applyRenderMask(smgr->getRootSceneNode(), mask, saved_visibility);
        }

        // Render the scene!
        smgr->drawAll();

        // Restore render mask visibilities
        for (auto &p : saved_visibility) {
            p.first->setVisible(p.second);
        }
    }

    // Restore Map culling state
    if (client_map && saved_map) {
        client_map->updateCamera(old_map_pos, old_map_dir, old_map_fov, old_map_offset, old_map_light);
        client_map->updateDrawList();
    }

    // Restore original render state
    driver->setRenderTarget(nullptr, false, false);
    driver->setViewPort(old_viewport);
    smgr->setActiveCamera(old_camera);
}
