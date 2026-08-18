// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include "render_target_impl.h"
#include "camera_impl.h"
#include "material_impl.h"
#include <IrrlichtDevice.h>
#include <IVideoDriver.h>
#include <ISceneManager.h>

namespace rendering
{

class IrrlichtShader : public IShader
{
public:
	IrrlichtShader(s32 materialType)
		: m_material_type(materialType)
	{
	}

	virtual ~IrrlichtShader() override = default;

	virtual bool isValid() const override { return m_material_type >= 0; }
	virtual void bind() override {}
	virtual void unbind() override {}

	virtual void setUniform(const std::string &name, float value) override {}
	virtual void setUniform(const std::string &name, int value) override {}
	virtual void setUniform(const std::string &name, const v2f &vec) override {}
	virtual void setUniform(const std::string &name, const v3f &vec) override {}
	virtual void setUniform(const std::string &name, const core::matrix4 &mat) override {}

	s32 getMaterialType() const { return m_material_type; }

private:
	s32 m_material_type {-1};
};

class IrrlichtContext : public IRenderContext
{
public:
	IrrlichtContext(video::IVideoDriver *driver, scene::ISceneManager *smgr)
		: m_driver(driver), m_smgr(smgr)
	{
	}

	virtual ~IrrlichtContext() override = default;

	virtual void beginFrame(video::SColor clearColor = video::SColor(255, 0, 0, 0)) override
	{
		if (m_driver)
			m_driver->beginScene(true, true, clearColor);
	}

	virtual void endFrame() override
	{
		if (m_driver)
			m_driver->endScene();
	}

	virtual void setRenderTarget(std::shared_ptr<IRenderTarget> target) override
	{
		m_target = target;
		if (m_target) {
			m_target->bind();
		} else if (m_driver) {
			m_driver->setRenderTargetEx(nullptr, video::ECBF_NONE);
		}
	}

	virtual std::shared_ptr<IRenderTarget> getRenderTarget() const override { return m_target; }

	virtual void setCamera(std::shared_ptr<ICamera> camera) override
	{
		m_camera = camera;
		if (m_camera && m_smgr) {
			auto irrCam = std::dynamic_pointer_cast<IrrlichtCamera>(m_camera);
			if (irrCam && irrCam->getIrrlichtCameraNode()) {
				m_smgr->setActiveCamera(irrCam->getIrrlichtCameraNode());
			}
		}
	}

	virtual std::shared_ptr<ICamera> getCamera() const override { return m_camera; }

	virtual void setViewport(const Viewport &viewport) override
	{
		m_viewport = viewport;
		if (m_driver) {
			m_driver->setViewPort(core::rect<s32>(viewport.x, viewport.y, viewport.x + viewport.width, viewport.y + viewport.height));
		}
	}

	virtual Viewport getViewport() const override { return m_viewport; }

	virtual void setMaterial(std::shared_ptr<IMaterial> material) override
	{
		m_material = material;
	}

	virtual void setShader(std::shared_ptr<IShader> shader) override
	{
		m_shader = shader;
	}

private:
	video::IVideoDriver *m_driver {nullptr};
	scene::ISceneManager *m_smgr {nullptr};
	std::shared_ptr<IRenderTarget> m_target;
	std::shared_ptr<ICamera> m_camera;
	Viewport m_viewport;
	std::shared_ptr<IMaterial> m_material;
	std::shared_ptr<IShader> m_shader;
};

class IrrlichtDevice : public IRenderDevice
{
public:
	IrrlichtDevice(::IrrlichtDevice *device)
		: m_device(device)
	{
		if (m_device) {
			m_driver = m_device->getVideoDriver();
			m_smgr = m_device->getSceneManager();
		}
	}

	virtual ~IrrlichtDevice() override = default;

	virtual bool hasFeature(DriverFeature feature) const override
	{
		if (!m_driver)
			return false;
		switch (feature) {
		case DriverFeature::RenderToTexture:
			return m_driver->queryFeature(video::EVDF_RENDER_TO_TARGET);
		case DriverFeature::MultipleRenderTargets:
			return m_driver->queryFeature(video::EVDF_MULTIPLE_RENDER_TARGETS);
		case DriverFeature::GeometryShader:
			return m_driver->queryFeature(video::EVDF_GEOMETRY_SHADER);
		default:
			return false;
		}
	}

	virtual std::string getVendorName() const override
	{
		return m_driver ? m_driver->getVendorInfo().c_str() : "";
	}

	virtual std::string getRendererName() const override
	{
		return m_driver ? m_driver->getName() : "";
	}

	virtual std::shared_ptr<IRenderTarget> createRenderTarget(u32 width, u32 height, TextureFormat colorFormat, bool withDepth = true) override
	{
		return std::make_shared<IrrlichtRenderTarget>(m_driver, width, height, colorFormat, withDepth);
	}

	virtual std::shared_ptr<ICamera> createCamera() override
	{
		return std::make_shared<IrrlichtCamera>(m_smgr);
	}

	virtual std::shared_ptr<IMaterial> createMaterial() override
	{
		return std::make_shared<IrrlichtMaterial>();
	}

	virtual std::shared_ptr<IShader> createShader(const std::string &vertexShader, const std::string &fragmentShader) override
	{
		return std::make_shared<IrrlichtShader>(0);
	}

private:
	::IrrlichtDevice *m_device {nullptr};
	video::IVideoDriver *m_driver {nullptr};
	scene::ISceneManager *m_smgr {nullptr};
};

} // namespace rendering
