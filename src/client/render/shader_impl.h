// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include "client/shader.h"
#include <IMaterialRendererServices.h>
#include <map>
#include <string>
#include <vector>

namespace rendering
{

class ShaderUniformStore : public IShaderUniformSetter
{
public:
	ShaderUniformStore() = default;
	virtual ~ShaderUniformStore() override = default;

	void setUniform(const std::string &name, float value)
	{
		m_floats[name] = value;
	}

	void setUniform(const std::string &name, int value)
	{
		m_ints[name] = value;
	}

	void setUniform(const std::string &name, const v2f &vec)
	{
		m_vec2s[name] = vec;
	}

	void setUniform(const std::string &name, const v3f &vec)
	{
		m_vec3s[name] = vec;
	}

	void setUniform(const std::string &name, const core::matrix4 &mat)
	{
		m_mats[name] = mat;
	}

	virtual void onSetUniforms(video::IMaterialRendererServices *services) override
	{
		if (!services)
			return;

		for (const auto &pair : m_floats) {
			s32 id = services->getPixelShaderConstantID(pair.first.c_str());
			if (id >= 0) {
				services->setPixelShaderConstant(id, &pair.second, 1);
			} else {
				id = services->getVertexShaderConstantID(pair.first.c_str());
				if (id >= 0)
					services->setVertexShaderConstant(id, &pair.second, 1);
			}
		}

		for (const auto &pair : m_ints) {
			s32 id = services->getPixelShaderConstantID(pair.first.c_str());
			if (id >= 0) {
				services->setPixelShaderConstant(id, &pair.second, 1);
			} else {
				id = services->getVertexShaderConstantID(pair.first.c_str());
				if (id >= 0)
					services->setVertexShaderConstant(id, &pair.second, 1);
			}
		}

		for (const auto &pair : m_vec2s) {
			float data[2] = {pair.second.X, pair.second.Y};
			s32 id = services->getPixelShaderConstantID(pair.first.c_str());
			if (id >= 0) {
				services->setPixelShaderConstant(id, data, 2);
			} else {
				id = services->getVertexShaderConstantID(pair.first.c_str());
				if (id >= 0)
					services->setVertexShaderConstant(id, data, 2);
			}
		}

		for (const auto &pair : m_vec3s) {
			float data[3] = {pair.second.X, pair.second.Y, pair.second.Z};
			s32 id = services->getPixelShaderConstantID(pair.first.c_str());
			if (id >= 0) {
				services->setPixelShaderConstant(id, data, 3);
			} else {
				id = services->getVertexShaderConstantID(pair.first.c_str());
				if (id >= 0)
					services->setVertexShaderConstant(id, data, 3);
			}
		}

		for (const auto &pair : m_mats) {
			s32 id = services->getPixelShaderConstantID(pair.first.c_str());
			if (id >= 0) {
				services->setPixelShaderConstant(id, pair.second.pointer(), 16);
			} else {
				id = services->getVertexShaderConstantID(pair.first.c_str());
				if (id >= 0)
					services->setVertexShaderConstant(id, pair.second.pointer(), 16);
			}
		}
	}

private:
	std::map<std::string, float> m_floats;
	std::map<std::string, int> m_ints;
	std::map<std::string, v2f> m_vec2s;
	std::map<std::string, v3f> m_vec3s;
	std::map<std::string, core::matrix4> m_mats;
};

class IrrlichtShader : public IShader
{
public:
	IrrlichtShader(s32 materialType)
		: m_material_type(materialType)
	{
		m_uniform_store = std::make_shared<ShaderUniformStore>();
	}

	virtual ~IrrlichtShader() override = default;

	virtual bool isValid() const override { return m_material_type >= 0; }
	virtual void bind() override {}
	virtual void unbind() override {}

	virtual void setUniform(const std::string &name, float value) override
	{
		m_uniform_store->setUniform(name, value);
	}

	virtual void setUniform(const std::string &name, int value) override
	{
		m_uniform_store->setUniform(name, value);
	}

	virtual void setUniform(const std::string &name, const v2f &vec) override
	{
		m_uniform_store->setUniform(name, vec);
	}

	virtual void setUniform(const std::string &name, const v3f &vec) override
	{
		m_uniform_store->setUniform(name, vec);
	}

	virtual void setUniform(const std::string &name, const core::matrix4 &mat) override
	{
		m_uniform_store->setUniform(name, mat);
	}

	s32 getMaterialType() const { return m_material_type; }
	std::shared_ptr<ShaderUniformStore> getUniformStore() const { return m_uniform_store; }

private:
	s32 m_material_type {-1};
	std::shared_ptr<ShaderUniformStore> m_uniform_store;
};

} // namespace rendering
