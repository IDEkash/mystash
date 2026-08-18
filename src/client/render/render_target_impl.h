// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include <IVideoDriver.h>
#include <IRenderTarget.h>
#include <ITexture.h>

namespace rendering
{

class IrrlichtRenderTarget : public IRenderTarget
{
public:
	IrrlichtRenderTarget(video::IVideoDriver *driver, u32 width, u32 height, TextureFormat format = TextureFormat::RGBA8, bool withDepth = true)
		: m_driver(driver), m_width(width), m_height(height)
	{
		if (!m_driver)
			return;

		m_irr_rt = m_driver->addRenderTarget();
		m_color_texture = m_driver->addRenderTargetTexture(core::dimension2du(width, height), "rt_color", video::ECF_A8R8G8B8);
		if (withDepth) {
			m_depth_texture = m_driver->addRenderTargetTexture(core::dimension2du(width, height), "rt_depth", video::ECF_D24S8);
		}

		if (m_irr_rt) {
			m_irr_rt->setTexture(m_color_texture, m_depth_texture);
		}
	}

	virtual ~IrrlichtRenderTarget() override
	{
		if (m_irr_rt && m_driver)
			m_driver->removeRenderTarget(m_irr_rt);
		if (m_color_texture && m_driver)
			m_driver->removeTexture(m_color_texture);
		if (m_depth_texture && m_driver)
			m_driver->removeTexture(m_depth_texture);
	}

	virtual u32 getWidth() const override { return m_width; }
	virtual u32 getHeight() const override { return m_height; }
	virtual v2u32 getSize() const override { return v2u32(m_width, m_height); }

	virtual void resize(u32 width, u32 height) override
	{
		if (m_width == width && m_height == height)
			return;

		m_width = width;
		m_height = height;

		if (m_color_texture && m_driver) {
			m_driver->removeTexture(m_color_texture);
			m_color_texture = m_driver->addRenderTargetTexture(core::dimension2du(width, height), "rt_color", video::ECF_A8R8G8B8);
		}
		if (m_depth_texture && m_driver) {
			m_driver->removeTexture(m_depth_texture);
			m_depth_texture = m_driver->addRenderTargetTexture(core::dimension2du(width, height), "rt_depth", video::ECF_D24S8);
		}
		if (m_irr_rt) {
			m_irr_rt->setTexture(m_color_texture, m_depth_texture);
		}
	}

	virtual void bind() override
	{
		if (m_driver && m_irr_rt) {
			m_driver->setRenderTargetEx(m_irr_rt, video::ECBF_ALL, video::SColor(255, 0, 0, 0));
			m_driver->OnResize(core::dimension2du(m_width, m_height));
		}
	}

	virtual void unbind() override
	{
		if (m_driver) {
			m_driver->setRenderTargetEx(nullptr, video::ECBF_NONE);
		}
	}

	virtual void *getNativeColorAttachment(u8 index = 0) const override
	{
		return static_cast<void*>(m_color_texture);
	}

	virtual void *getNativeDepthAttachment() const override
	{
		return static_cast<void*>(m_depth_texture);
	}

	video::IRenderTarget *getIrrlichtRenderTarget() const { return m_irr_rt; }

private:
	video::IVideoDriver *m_driver {nullptr};
	video::IRenderTarget *m_irr_rt {nullptr};
	video::ITexture *m_color_texture {nullptr};
	video::ITexture *m_depth_texture {nullptr};
	u32 m_width {0};
	u32 m_height {0};
};

} // namespace rendering
