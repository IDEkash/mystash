// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "unapi_object.h"
#include "unapi_types.h"
#include "irrlichttypes.h"
#include <rect.h>
#include <memory>
#include <string>
#include <vector>

#if IS_CLIENT_BUILD
namespace irr {
namespace video {
	class IRenderTarget;
	class ITexture;
}
}
#endif

namespace unapi {

// Render Foundation
class RenderCamera : public UnapiObject {
private:
	Vector3f m_position {0, 0, 0};
	Vector3f m_rotation {0, 0, 0};
	float m_fov {72.0f};
	float m_aspect {1.777f};
	float m_near {0.1f};
	float m_far {1000.0f};
	bool m_enabled {true};
	ObjectRefPtr m_world;

public:
	RenderCamera();
	Vector3f getPosition() const { return m_position; }
	void setPosition(Vector3f p) { m_position = p; }
	Vector3f getRotation() const { return m_rotation; }
	void setRotation(Vector3f r) { m_rotation = r; }
	float getFov() const { return m_fov; }
	void setFov(float f) { m_fov = f; }
	float getAspectRatio() const { return m_aspect; }
	void setAspectRatio(float a) { m_aspect = a; }
	float getNearPlane() const { return m_near; }
	void setNearPlane(float n) { m_near = n; }
	float getFarPlane() const { return m_far; }
	void setFarPlane(float f) { m_far = f; }
	bool isEnabled() const { return m_enabled; }
	void setEnabled(bool e) { m_enabled = e; }
	ObjectRefPtr getWorld() const { return m_world; }
	void setWorld(ObjectRefPtr w) { m_world = w; }
	Value renderView();
};

class RenderTextureObject : public UnapiObject {
private:
	std::string m_name {"texture_ext"};
	int m_width {1024};
	int m_height {1024};
	std::string m_format {"RGBA8"};
	std::vector<uint8_t> m_buffer;

public:
	RenderTextureObject();
	const std::string &getName() const { return m_name; }
	void setName(const std::string &n) { m_name = n; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	void setDimensions(int w, int h) { m_width = w; m_height = h; }
	void updateData(const std::vector<uint8_t> &data) { m_buffer = data; }
	const std::vector<uint8_t> &getData() const { return m_buffer; }
};

class RenderTargetObject : public UnapiObject {
private:
	int m_width {1024};
	int m_height {1024};
	std::string m_format {"RGBA8"};
	Vector3f m_clear_color {0, 0, 0};
	ObjectRefPtr m_texture;
	std::string m_texture_name;

#if IS_CLIENT_BUILD
	irr::video::IRenderTarget *m_irr_render_target {nullptr};
	irr::video::ITexture *m_irr_texture {nullptr};
	irr::video::IRenderTarget *m_saved_render_target {nullptr};
	core::rect<s32> m_saved_viewport;
#endif

public:
	RenderTargetObject();
	~RenderTargetObject() override;
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	void resize(int w, int h);
	ObjectRefPtr getTexture() const { return m_texture; }
	const std::string &getTextureName() const { return m_texture_name; }
	bool bindTarget();
	bool unbindTarget();
};

class RenderPassObject : public UnapiObject {
private:
	ObjectRefPtr m_camera;
	ObjectRefPtr m_target;
	ObjectRefPtr m_world;
	bool m_enabled {true};
	int m_clear_flags {3}; // depth + color
	int m_recursion_depth {0};
	int m_max_recursion_depth {3};

public:
	RenderPassObject();
	Value executePass();
	void setCamera(ObjectRefPtr cam) { m_camera = cam; }
	void setTarget(ObjectRefPtr tgt) { m_target = tgt; }
	void setWorld(ObjectRefPtr wld) { m_world = wld; }
	ObjectRefPtr getCamera() const { return m_camera; }
	ObjectRefPtr getTarget() const { return m_target; }
	ObjectRefPtr getWorld() const { return m_world; }
	int getRecursionDepth() const { return m_recursion_depth; }
	void setRecursionDepth(int d) { m_recursion_depth = d; }
	int getMaxRecursionDepth() const { return m_max_recursion_depth; }
	void setMaxRecursionDepth(int m) { m_max_recursion_depth = m; }
};

class RenderShaderObject : public UnapiObject {
private:
	std::string m_name;
	std::string m_vertex_src;
	std::string m_fragment_src;
	std::unordered_map<std::string, float> m_uniforms_float;
	std::unordered_map<std::string, Vector3f> m_uniforms_vector;

public:
	RenderShaderObject();
	void bindShader();
	void setUniformFloat(const std::string &name, float val) { m_uniforms_float[name] = val; }
	void setUniformVector(const std::string &name, Vector3f vec) { m_uniforms_vector[name] = vec; }
};

class RenderMeshObject : public UnapiObject {
private:
	int m_vertex_count {0};
	int m_index_count {0};
	std::vector<float> m_vertices;
	std::vector<uint32_t> m_indices;

public:
	RenderMeshObject();
	void setVertices(const std::vector<float> &v) { m_vertices = v; m_vertex_count = static_cast<int>(v.size() / 3); }
	void setIndices(const std::vector<uint32_t> &i) { m_indices = i; m_index_count = static_cast<int>(i.size()); }
	int getVertexCount() const { return m_vertex_count; }
	int getIndexCount() const { return m_index_count; }
};

class RenderVisualStateObject : public UnapiObject {
private:
	Vector3f m_sky_color {0.5f, 0.7f, 1.0f};
	float m_fog_distance {1000.0f};
	float m_light_override {1.0f};

public:
	RenderVisualStateObject();
	void applyState();
};

// Gameplay Foundation
class GameWorldObject : public UnapiObject {
private:
	std::string m_name {"world_main"};
	float m_time_of_day {0.5f};
	float m_gravity {9.81f};
	int m_instance_id {0};
	std::vector<ObjectRefPtr> m_entities;

public:
	GameWorldObject();
	const std::string &getName() const { return m_name; }
	void setName(const std::string &n) { m_name = n; }
	int getInstanceId() const { return m_instance_id; }
	void setInstanceId(int id) { m_instance_id = id; }
	ObjectRefPtr spawnEntity(const std::string &type, Vector3f pos);
	std::vector<ObjectRefPtr> getEntities() const { return m_entities; }
};

class GameEntityObject : public UnapiObject {
private:
	int64_t m_id {0};
	Vector3f m_position {0, 0, 0};
	Vector3f m_rotation {0, 0, 0};
	Vector3f m_velocity {0, 0, 0};
	float m_yaw {0.0f};
	ObjectRefPtr m_model;

public:
	GameEntityObject();
	Vector3f getPosition() const { return m_position; }
	void setPosition(Vector3f p) { m_position = p; }
	Vector3f getRotation() const { return m_rotation; }
	void setRotation(Vector3f r) { m_rotation = r; }
	float getYaw() const { return m_yaw; }
	void setYaw(float y) { m_yaw = y; }
	void move(Vector3f delta) { m_position.x += delta.x; m_position.y += delta.y; m_position.z += delta.z; }
};

class GamePlayerObject : public UnapiObject {
private:
	std::string m_name {"player"};
	Vector3f m_position {0, 0, 0};
	Vector3f m_rotation {0, 0, 0};
	ObjectRefPtr m_camera;

public:
	GamePlayerObject();
};

class GamePhysicsObject : public UnapiObject {
private:
	float m_gravity {9.81f};
	float m_step_time {0.016f};

public:
	GamePhysicsObject();
	void stepPhysics(float dtime);
};

// Asset Foundation
class AssetSkeletonObject : public UnapiObject {
private:
	std::unordered_map<std::string, TransformMatrix> m_bone_transforms;

public:
	AssetSkeletonObject();
	TransformMatrix getBoneTransform(const std::string &name) const;
	void setBoneTransform(const std::string &name, TransformMatrix tf) { m_bone_transforms[name] = tf; }
};

class AssetModelObject : public UnapiObject {
private:
	std::string m_path;
	ObjectRefPtr m_skeleton;

public:
	AssetModelObject();
	ObjectRefPtr getSkeleton() const { return m_skeleton; }
};

class AssetAnimationObject : public UnapiObject {
private:
	std::string m_name;
	float m_speed {1.0f};
	bool m_loop {true};
	bool m_playing {false};

public:
	AssetAnimationObject();
	void play() { m_playing = true; }
	void stop() { m_playing = false; }
	bool isPlaying() const { return m_playing; }
};

class AssetMaterialObject : public UnapiObject {
private:
	std::string m_name;
	ObjectRefPtr m_texture;
	ObjectRefPtr m_shader;

public:
	AssetMaterialObject();
	void setTexture(ObjectRefPtr tex) { m_texture = tex; }
	ObjectRefPtr getTexture() const { return m_texture; }
	void setShader(ObjectRefPtr sh) { m_shader = sh; }
	ObjectRefPtr getShader() const { return m_shader; }
};

class AssetTextureObject : public UnapiObject {
private:
	std::string m_path;
	int m_width {0};
	int m_height {0};

public:
	AssetTextureObject();
};

void registerAllFoundationTypes();

} // namespace unapi
