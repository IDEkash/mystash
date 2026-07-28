#pragma once

#include "irrlichttypes_bloated.h"
#include <rect.h>
#include <vector3d.h>
#include <SColor.h>
#include <vector>
#include <string>

namespace video {
    class ITexture;
}
namespace scene {
    class ICameraSceneNode;
    class ISceneNode;
}

class Client;

class GPURenderTarget {
public:
    GPURenderTarget(u32 width, u32 height, const std::string &name, const std::string &format_str);
    ~GPURenderTarget();

    video::ITexture* getTexture() const { return m_texture; }
    std::string getName() const { return m_name; }
    u32 getWidth() const { return m_width; }
    u32 getHeight() const { return m_height; }

private:
    video::ITexture *m_texture = nullptr;
    std::string m_name;
    u32 m_width;
    u32 m_height;
};

class RenderCamera {
public:
    RenderCamera();
    ~RenderCamera();

    void setPos(const v3f &pos);
    v3f getPos() const;

    void setRotation(const v3f &rot);
    v3f getRotation() const;

    void setFov(f32 fov);
    f32 getFov() const { return m_fov; }

    void setProjection(const std::string &proj);
    std::string getProjection() const { return m_projection; }

    void setNearFar(f32 near_plane, f32 far_plane);
    f32 getNear() const { return m_near_plane; }
    f32 getFar() const { return m_far_plane; }

    void setViewport(const core::rect<f32> &viewport);
    core::rect<f32> getViewport() const { return m_viewport; }

    void setRenderPriority(s32 priority) { m_priority = priority; }
    s32 getRenderPriority() const { return m_priority; }

    void setRenderTarget(GPURenderTarget *target) { m_render_target = target; }
    GPURenderTarget* getRenderTarget() const { return m_render_target; }

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool getEnabled() const { return m_enabled; }

    void setUpdateFrequency(f32 freq) { m_update_frequency = freq; }
    f32 getUpdateFrequency() const { return m_update_frequency; }

    void setRenderMask(u32 mask) { m_render_mask = mask; }
    u32 getRenderMask() const { return m_render_mask; }

    void setResolutionScaling(f32 scale) { m_resolution_scaling = scale; }
    f32 getResolutionScaling() const { return m_resolution_scaling; }

    bool shouldRender(f32 dtime);

    void setParent(scene::ISceneNode *parent);
    scene::ISceneNode *getParent() const;

    scene::ICameraSceneNode *getIrrlichtCameraNode() const { return m_cameranode; }

private:
    scene::ICameraSceneNode *m_cameranode = nullptr;
    f32 m_fov = 70.0f;
    std::string m_projection = "perspective";
    f32 m_near_plane = 0.1f;
    f32 m_far_plane = 1000.0f;
    core::rect<f32> m_viewport = core::rect<f32>(0, 0, 1, 1);
    s32 m_priority = 0;
    bool m_enabled = true;
    GPURenderTarget *m_render_target = nullptr;
    f32 m_update_frequency = 0.0f;
    f32 m_time_since_last_render = 0.0f;
    u32 m_render_mask = 0xFFFFFFFF;
    f32 m_resolution_scaling = 1.0f;
};

class RenderCameraManager {
public:
    static RenderCameraManager* get();

    void registerCamera(RenderCamera *camera);
    void unregisterCamera(RenderCamera *camera);

    void registerRenderTarget(GPURenderTarget *target);
    void unregisterRenderTarget(GPURenderTarget *target);

    const std::vector<RenderCamera*>& getCameras() const { return m_cameras; }
    const std::vector<GPURenderTarget*>& getRenderTargets() const { return m_render_targets; }

    void setClient(Client *client) { m_client = client; }
    Client* getClient() const { return m_client; }

    void clear();

    void renderAll(video::SColor clear_color, bool offscreen_only);

private:
    RenderCameraManager() = default;
    ~RenderCameraManager() = default;

    std::vector<RenderCamera*> m_cameras;
    std::vector<GPURenderTarget*> m_render_targets;
    Client *m_client = nullptr;
};
