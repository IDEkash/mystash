#pragma once
#include "irrlichttypes_bloated.h"
#include "irr_ptr.h"
#include <string>
#include <vector>
#include <map>
#include "client/shader.h"
#include "client/tile.h"

class LuaShaderUniformSetter : public IShaderUniformSetterRC
{
public:
	void onSetUniforms(video::IMaterialRendererServices *services) override
	{
		for (auto& it : m_f1)
			services->setPixelShaderConstant(getID(services, it.second, it.first), &it.second.value, 1);
		for (auto& it : m_f2) {
			float f[2] = {it.second.value.X, it.second.value.Y};
			services->setPixelShaderConstant(getID(services, it.second, it.first), f, 2);
		}
		for (auto& it : m_f3) {
			float f[3] = {it.second.value.X, it.second.value.Y, it.second.value.Z};
			services->setPixelShaderConstant(getID(services, it.second, it.first), f, 3);
		}
		for (auto& it : m_f4) {
			float f[4] = {it.second.value.r, it.second.value.g, it.second.value.b, it.second.value.a};
			services->setPixelShaderConstant(getID(services, it.second, it.first), f, 4);
		}
		for (auto& it : m_m4)
			services->setPixelShaderConstant(getID(services, it.second, it.first), it.second.value.pointer(), 16);
	}

	void setUniform(const std::string &name, float value) { m_f1[name].value = value; m_f1[name].id = -1; }
	void setUniform(const std::string &name, v2f value) { m_f2[name].value = value; m_f2[name].id = -1; }
	void setUniform(const std::string &name, v3f value) { m_f3[name].value = value; m_f3[name].id = -1; }
	void setUniform(const std::string &name, video::SColorf value) { m_f4[name].value = value; m_f4[name].id = -1; }
	void setUniform(const std::string &name, const core::matrix4 &value) { m_m4[name].value = value; m_m4[name].id = -1; }

	void clear() {
		m_f1.clear();
		m_f2.clear();
		m_f3.clear();
		m_f4.clear();
		m_m4.clear();
	}

	void clearCache() {
		for (auto& it : m_f1) it.second.id = -1;
		for (auto& it : m_f2) it.second.id = -1;
		for (auto& it : m_f3) it.second.id = -1;
		for (auto& it : m_f4) it.second.id = -1;
		for (auto& it : m_m4) it.second.id = -1;
	}

private:
	template<typename T>
	struct UniformValue {
		T value;
		mutable s32 id = -1;
	};

	template<typename T>
	s32 getID(video::IMaterialRendererServices *services, UniformValue<T> const& uv, const std::string &name) const {
		if (uv.id == -1) uv.id = services->getPixelShaderConstantID(name.c_str());
		return uv.id;
	}

	std::map<std::string, UniformValue<float>> m_f1;
	std::map<std::string, UniformValue<v2f>> m_f2;
	std::map<std::string, UniformValue<v3f>> m_f3;
	std::map<std::string, UniformValue<video::SColorf>> m_f4;
	std::map<std::string, UniformValue<core::matrix4>> m_m4;
};

struct CustomPass {
	u32 id;
	std::string shader_name;
	std::vector<u8> texture_map;
	u32 shader_id;
	irr_ptr<LuaShaderUniformSetter> setter;

	CustomPass() = default;
	CustomPass(const CustomPass &other) :
		id(other.id), shader_name(other.shader_name), texture_map(other.texture_map),
		shader_id(other.shader_id), setter(other.setter) {}
	CustomPass(CustomPass &&) = default;
	CustomPass& operator=(const CustomPass &other) {
		id = other.id;
		shader_name = other.shader_name;
		texture_map = other.texture_map;
		shader_id = other.shader_id;
		setter = other.setter;
		return *this;
	}
	CustomPass& operator=(CustomPass &&) = default;
};

class VisualsManager {
public:
	u32 addPass(const std::string &shader_name, const std::vector<u8> &texture_map, u32 shader_id, LuaShaderUniformSetter *setter)
	{
		u32 id = m_next_id++;
		CustomPass pass;
		pass.id = id;
		pass.shader_name = shader_name;
		pass.texture_map = texture_map;
		pass.shader_id = shader_id;
		pass.setter.grab(setter);
		m_passes.push_back(std::move(pass));
		return id;
	}

	void removePass(u32 id)
	{
		for (auto it = m_passes.begin(); it != m_passes.end(); ++it) {
			if (it->id == id) {
				m_passes.erase(it);
				return;
			}
		}
	}

	const std::vector<CustomPass>& getPasses() const { return m_passes; }

	CustomPass* getPass(u32 id) {
		for (auto &pass : m_passes) {
			if (pass.id == id) return &pass;
		}
		return nullptr;
	}

private:
	std::vector<CustomPass> m_passes;
	u32 m_next_id = 1;
};
