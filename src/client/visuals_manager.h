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
		for (auto& [name, val] : m_f1)
			services->setPixelShaderConstant(getID(services, name), &val.value, 1);
		for (auto& [name, val] : m_f2) {
			float f[2] = {val.value.X, val.value.Y};
			services->setPixelShaderConstant(getID(services, name), f, 2);
		}
		for (auto& [name, val] : m_f3) {
			float f[3] = {val.value.X, val.value.Y, val.value.Z};
			services->setPixelShaderConstant(getID(services, name), f, 3);
		}
		for (auto& [name, val] : m_f4) {
			float f[4] = {val.value.r, val.value.g, val.value.b, val.value.a};
			services->setPixelShaderConstant(getID(services, name), f, 4);
		}
		for (auto& [name, val] : m_m4)
			services->setPixelShaderConstant(getID(services, name), val.value.pointer(), 16);
	}

	void setUniform(const std::string &name, float value) { m_f1[name].value = value; }
	void setUniform(const std::string &name, v2f value) { m_f2[name].value = value; }
	void setUniform(const std::string &name, v3f value) { m_f3[name].value = value; }
	void setUniform(const std::string &name, video::SColorf value) { m_f4[name].value = value; }
	void setUniform(const std::string &name, const core::matrix4 &value) { m_m4[name].value = value; }

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

	s32 getID(video::IMaterialRendererServices *services, const std::string &name) const {
		// We don't really have a good way to cache ID across DIFFERENT services/shaders
		// but usually it's the same shader for this setter.
		// However, I'll just use a local mutable cache.
		return services->getPixelShaderConstantID(name.c_str());
	}

	// Re-evaluating the ID caching. getPixelShaderConstantID is slow.
	// But it returns -1 if not found.
	// If we cache it, we must ensure it's still valid for the current shader.
	// Since IShaderUniformSetter is attached to a shader material, and
	// CustomPostProcessingStep uses it, it should be fine as long as the material
	// doesn't change.

	s32 getID(video::IMaterialRendererServices *services, UniformValue<float> const& uv, const std::string &name) const {
		if (uv.id == -1) uv.id = services->getPixelShaderConstantID(name.c_str());
		return uv.id;
	}
	// Redefining onSetUniforms to use this properly.

public:
	void onSetUniformsFixed(video::IMaterialRendererServices *services) {
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
	// Override the previous one
	void onSetUniforms(video::IMaterialRendererServices *services) override {
		onSetUniformsFixed(services);
	}

private:
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
};

class VisualsManager {
public:
	u32 addPass(const std::string &shader_name, const std::vector<u8> &texture_map, u32 shader_id, LuaShaderUniformSetter *setter)
	{
		u32 id = m_next_id++;
		m_passes.push_back({id, shader_name, texture_map, shader_id, {setter}});
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
	std::vector<CustomPass> m_passes; // Use vector to preserve order for chaining
	u32 m_next_id = 1;
};
