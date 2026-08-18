// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include "client/shader.h"
#include <SMaterial.h>
#include <IMaterialRendererServices.h>

namespace rendering
{

class PBRShaderUniformSetter : public IShaderUniformSetter
{
public:
	PBRShaderUniformSetter() = default;
	virtual ~PBRShaderUniformSetter() override = default;

	void setRoughness(f32 roughness) { m_roughness = roughness; }
	void setMetallic(f32 metallic) { m_metallic = metallic; }
	void setEmissiveColor(const video::SColor &color) { m_emissive = color; }

	virtual void onSetMaterial(const video::SMaterial &material) override
	{
		m_emissive = material.EmissiveColor;
	}

	virtual void onSetUniforms(video::IMaterialRendererServices *services) override
	{
		if (!services)
			return;

		s32 roughnessID = services->getPixelShaderConstantID("u_Roughness");
		if (roughnessID >= 0)
			services->setPixelShaderConstant(roughnessID, &m_roughness, 1);

		s32 metallicID = services->getPixelShaderConstantID("u_Metallic");
		if (metallicID >= 0)
			services->setPixelShaderConstant(metallicID, &m_metallic, 1);

		video::SColorf emissivef(m_emissive);
		float emissiveData[4] = {emissivef.r, emissivef.g, emissivef.b, emissivef.a};
		s32 emissiveID = services->getPixelShaderConstantID("u_EmissiveColor");
		if (emissiveID >= 0)
			services->setPixelShaderConstant(emissiveID, emissiveData, 4);
	}

private:
	f32 m_roughness {0.5f};
	f32 m_metallic {0.0f};
	video::SColor m_emissive {255, 0, 0, 0};
};

class IrrlichtMaterial : public IMaterial
{
public:
	IrrlichtMaterial()
	{
		m_pbr_setter = std::make_shared<PBRShaderUniformSetter>();
	}

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

	virtual void setRoughness(f32 roughness) override
	{
		m_roughness = roughness;
		m_pbr_setter->setRoughness(roughness);
	}

	virtual f32 getRoughness() const override { return m_roughness; }

	virtual void setMetallic(f32 metallic) override
	{
		m_metallic = metallic;
		m_pbr_setter->setMetallic(metallic);
	}

	virtual f32 getMetallic() const override { return m_metallic; }

	virtual void setEmissiveColor(video::SColor color) override
	{
		m_emissive = color;
		m_material.EmissiveColor = color;
		m_pbr_setter->setEmissiveColor(color);
	}

	virtual video::SColor getEmissiveColor() const override { return m_emissive; }

	virtual void setTexture(u8 slot, void *nativeTextureHandle) override
	{
		if (slot < video::MATERIAL_MAX_TEXTURES) {
			m_material.setTexture(slot, static_cast<video::ITexture *>(nativeTextureHandle));
		}
	}

	video::SMaterial &getIrrlichtMaterial() { return m_material; }
	std::shared_ptr<PBRShaderUniformSetter> getPBRSetter() const { return m_pbr_setter; }

private:
	video::SMaterial m_material;
	BlendMode m_blend_mode {BlendMode::Opaque};
	f32 m_roughness {0.5f};
	f32 m_metallic {0.0f};
	video::SColor m_emissive {255, 0, 0, 0};
	std::shared_ptr<PBRShaderUniformSetter> m_pbr_setter;
};

} // namespace rendering
