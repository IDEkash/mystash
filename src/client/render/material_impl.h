// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include <SMaterial.h>

namespace rendering
{

class IrrlichtMaterial : public IMaterial
{
public:
	IrrlichtMaterial() = default;
	virtual ~IrrlichtMaterial() override = default;

	virtual void setBlendMode(BlendMode mode) override
	{
		m_blend_mode = mode;
		switch (mode) {
		case BlendMode::Opaque:
			m_material.MaterialType = video::EMT_SOLID;
			break;
		case BlendMode::Alpha:
			m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			break;
		case BlendMode::Add:
			m_material.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
			break;
		default:
			m_material.MaterialType = video::EMT_SOLID;
			break;
		}
	}

	virtual BlendMode getBlendMode() const override { return m_blend_mode; }

	virtual void setRoughness(f32 roughness) override { m_roughness = roughness; }
	virtual f32 getRoughness() const override { return m_roughness; }

	virtual void setMetallic(f32 metallic) override { m_metallic = metallic; }
	virtual f32 getMetallic() const override { return m_metallic; }

	virtual void setEmissiveColor(video::SColor color) override
	{
		m_emissive = color;
		m_material.EmissiveColor = color;
	}

	virtual video::SColor getEmissiveColor() const override { return m_emissive; }

	virtual void setTexture(u8 slot, void *nativeTextureHandle) override
	{
		if (slot < video::MATERIAL_MAX_TEXTURES) {
			m_material.setTexture(slot, static_cast<video::ITexture *>(nativeTextureHandle));
		}
	}

	video::SMaterial &getIrrlichtMaterial() { return m_material; }

private:
	video::SMaterial m_material;
	BlendMode m_blend_mode {BlendMode::Opaque};
	f32 m_roughness {0.5f};
	f32 m_metallic {0.0f};
	video::SColor m_emissive {255, 0, 0, 0};
};

} // namespace rendering
