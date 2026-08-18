// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "irr_v3d.h"
#include "matrix4.h"
#include <SColor.h>
#include <string>
#include <vector>
#include <memory>

namespace rendering
{

enum class DriverFeature
{
	RenderToTexture,
	MultipleRenderTargets,
	GeometryShader,
	ComputeShader,
	InstancedRendering,
	GPUSkinning,
	PBRMaterials,
	DepthTexture
};

enum class TextureFormat
{
	RGBA8,
	RGB8,
	RGBA16F,
	RGBA32F,
	Depth16,
	Depth24Stencil8
};

enum class BlendMode
{
	Opaque,
	Alpha,
	Add,
	Multiply,
	Screen
};

struct Viewport
{
	s32 x {0};
	s32 y {0};
	u32 width {0};
	u32 height {0};
};

class IRenderTarget;
class ICamera;
class IMaterial;
class IShader;
class IMesh;

/**
 * Modern GPU Render Device interface encapsulating low-level graphics backend capabilities.
 */
class IRenderDevice
{
public:
	virtual ~IRenderDevice() = default;

	virtual bool hasFeature(DriverFeature feature) const = 0;
	virtual std::string getVendorName() const = 0;
	virtual std::string getRendererName() const = 0;

	virtual std::shared_ptr<IRenderTarget> createRenderTarget(u32 width, u32 height, TextureFormat colorFormat, bool withDepth = true) = 0;
	virtual std::shared_ptr<ICamera> createCamera() = 0;
	virtual std::shared_ptr<IMaterial> createMaterial() = 0;
	virtual std::shared_ptr<IShader> createShader(const std::string &vertexShader, const std::string &fragmentShader) = 0;
};

/**
 * Rendering Context representing the state for active rendering passes/frames.
 */
class IRenderContext
{
public:
	virtual ~IRenderContext() = default;

	virtual void beginFrame(video::SColor clearColor = video::SColor(255, 0, 0, 0)) = 0;
	virtual void endFrame() = 0;

	virtual void setRenderTarget(std::shared_ptr<IRenderTarget> target) = 0;
	virtual std::shared_ptr<IRenderTarget> getRenderTarget() const = 0;

	virtual void setCamera(std::shared_ptr<ICamera> camera) = 0;
	virtual std::shared_ptr<ICamera> getCamera() const = 0;

	virtual void setViewport(const Viewport &viewport) = 0;
	virtual Viewport getViewport() const = 0;

	virtual void setMaterial(std::shared_ptr<IMaterial> material) = 0;
	virtual void setShader(std::shared_ptr<IShader> shader) = 0;
};

/**
 * Offscreen Render Target / Framebuffer Interface supporting modern RTT and multi-pass output.
 */
class IRenderTarget
{
public:
	virtual ~IRenderTarget() = default;

	virtual u32 getWidth() const = 0;
	virtual u32 getHeight() const = 0;
	virtual v2u32 getSize() const = 0;

	virtual void resize(u32 width, u32 height) = 0;
	virtual void bind() = 0;
	virtual void unbind() = 0;

	virtual void *getNativeColorAttachment(u8 index = 0) const = 0;
	virtual void *getNativeDepthAttachment() const = 0;
};

/**
 * Decoupled Camera interface for multi-camera, portal, secondary viewport, and reflection rendering.
 */
class ICamera
{
public:
	virtual ~ICamera() = default;

	virtual void setPosition(const v3f &pos) = 0;
	virtual v3f getPosition() const = 0;

	virtual void setTarget(const v3f &target) = 0;
	virtual v3f getTarget() const = 0;

	virtual void setUpVector(const v3f &up) = 0;
	virtual v3f getUpVector() const = 0;

	virtual void setFOV(f32 fovDegrees) = 0;
	virtual f32 getFOV() const = 0;

	virtual void setAspectRatio(f32 aspect) = 0;
	virtual f32 getAspectRatio() const = 0;

	virtual void setNearPlane(f32 nearVal) = 0;
	virtual f32 getNearPlane() const = 0;

	virtual void setFarPlane(f32 farVal) = 0;
	virtual f32 getFarPlane() const = 0;

	virtual const core::matrix4 &getViewMatrix() = 0;
	virtual const core::matrix4 &getProjectionMatrix() = 0;
	virtual core::matrix4 getViewProjectionMatrix() = 0;
};

/**
 * Modern Material interface supporting standard, metallic-roughness PBR, blend states, and custom textures.
 */
class IMaterial
{
public:
	virtual ~IMaterial() = default;

	virtual void setBlendMode(BlendMode mode) = 0;
	virtual BlendMode getBlendMode() const = 0;

	virtual void setRoughness(f32 roughness) = 0;
	virtual f32 getRoughness() const = 0;

	virtual void setMetallic(f32 metallic) = 0;
	virtual f32 getMetallic() const = 0;

	virtual void setEmissiveColor(video::SColor color) = 0;
	virtual video::SColor getEmissiveColor() const = 0;

	virtual void setTexture(u8 slot, void *nativeTextureHandle) = 0;
};

/**
 * Shader interface for uniform management and dynamic pipeline binding.
 */
class IShader
{
public:
	virtual ~IShader() = default;

	virtual bool isValid() const = 0;
	virtual void bind() = 0;
	virtual void unbind() = 0;

	virtual void setUniform(const std::string &name, float value) = 0;
	virtual void setUniform(const std::string &name, int value) = 0;
	virtual void setUniform(const std::string &name, const v2f &vec) = 0;
	virtual void setUniform(const std::string &name, const v3f &vec) = 0;
	virtual void setUniform(const std::string &name, const core::matrix4 &mat) = 0;
};

/**
 * Mesh interface encapsulating geometry buffers, vertex attributes, and instanced draw calls.
 */
class IMesh
{
public:
	virtual ~IMesh() = default;

	virtual u32 getVertexCount() const = 0;
	virtual u32 getIndexCount() const = 0;
	virtual void render(bool instanced = false, u32 instanceCount = 1) = 0;
};

/**
 * Modular Render Pass step interface for render pipelines.
 */
class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	virtual const std::string &getName() const = 0;
	virtual bool isEnabled() const = 0;
	virtual void setEnabled(bool enabled) = 0;

	virtual void prepare(IRenderContext *context) = 0;
	virtual void execute(IRenderContext *context) = 0;
	virtual void cleanup(IRenderContext *context) = 0;
};

} // namespace rendering
