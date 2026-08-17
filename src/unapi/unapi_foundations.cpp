// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_foundations.h"
#include "unapi_registry.h"
#include "unapi_hooks.h"
#include "log.h"

#if IS_CLIENT_BUILD
#include "client/renderingengine.h"
#include <IVideoDriver.h>
#include <ISceneManager.h>
#include <ICameraSceneNode.h>
#endif

namespace unapi {

// RenderCamera Implementation
RenderCamera::RenderCamera() : UnapiObject("render.camera") {
	registerGetter("position", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_position);
	});
	registerSetter("position", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_position = v.asVector3();
		return true;
	});

	registerGetter("rotation", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_rotation);
	});
	registerSetter("rotation", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_rotation = v.asVector3();
		return true;
	});

	registerGetter("fov", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_fov);
	});
	registerSetter("fov", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_fov = static_cast<float>(v.asFloat(72.0));
		return true;
	});

	registerGetter("near_plane", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_near);
	});
	registerSetter("near_plane", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_near = static_cast<float>(v.asFloat(0.1));
		return true;
	});

	registerGetter("far_plane", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_far);
	});
	registerSetter("far_plane", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_far = static_cast<float>(v.asFloat(1000.0));
		return true;
	});

	registerGetter("enabled", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_enabled);
	});
	registerSetter("enabled", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_enabled = v.asBool(true);
		return true;
	});

	registerGetter("world", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderCamera*>(obj)->m_world);
	});
	registerSetter("world", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderCamera*>(obj)->m_world = v.asObject();
		return true;
	});

	registerMethod("set_transform", [](UnapiObject *obj, const ValueArray &args) {
		auto cam = static_cast<RenderCamera*>(obj);
		if (args.size() >= 1) cam->m_position = args[0].asVector3();
		if (args.size() >= 2) cam->m_rotation = args[1].asVector3();
		return Value(true);
	});

	registerMethod("look_at", [](UnapiObject *obj, const ValueArray &args) {
		auto cam = static_cast<RenderCamera*>(obj);
		if (args.size() >= 1) {
			Vector3f target = args[0].asVector3();
			Vector3f dir(target.x - cam->m_position.x, target.y - cam->m_position.y, target.z - cam->m_position.z);
			cam->m_rotation = dir;
		}
		return Value(true);
	});

	registerMethod("render", [](UnapiObject *obj, const ValueArray &args) {
		return static_cast<RenderCamera*>(obj)->renderView();
	});
}

struct SavedCameraState {
	v3f pos;
	v3f target;
	v3f up;
	f32 fov {1.0f};
	f32 near_plane {0.1f};
	f32 far_plane {1000.0f};
	f32 aspect {1.777f};
};

Value RenderCamera::renderView() {
	if (!m_enabled) return Value(false);

#if IS_CLIENT_BUILD
	auto driver = RenderingEngine::get_video_driver();
	auto smgr = RenderingEngine::get_raw_device() ? RenderingEngine::get_raw_device()->getSceneManager() : nullptr;
	SavedCameraState saved;
	bool camera_saved = false;

	if (driver && smgr) {
		scene::ICameraSceneNode *cam = smgr->getActiveCamera();
		if (cam) {
			saved.pos = cam->getPosition();
			saved.target = cam->getTarget();
			saved.up = cam->getUpVector();
			saved.fov = cam->getFOV();
			saved.near_plane = cam->getNearValue();
			saved.far_plane = cam->getFarValue();
			saved.aspect = cam->getAspectRatio();
			camera_saved = true;

			cam->setPosition(v3f(m_position.x, m_position.y, m_position.z));
			cam->setTarget(v3f(m_position.x + m_rotation.x, m_position.y + m_rotation.y, m_position.z + m_rotation.z));
			cam->setFOV(m_fov * M_PI / 180.0f);
			cam->setNearValue(m_near);
			cam->setFarValue(m_far);
			cam->setAspectRatio(m_aspect);
			cam->updateAbsolutePosition();
		}
	}
#endif

	ValueArray hook_args;
	hook_args.push_back(Value(getHandleId()));
	HookRegistry::get().executeHooks("render.camera.render", hook_args);

#if IS_CLIENT_BUILD
	if (driver && smgr && camera_saved) {
		scene::ICameraSceneNode *cam = smgr->getActiveCamera();
		if (cam) {
			cam->setPosition(saved.pos);
			cam->setTarget(saved.target);
			cam->setUpVector(saved.up);
			cam->setFOV(saved.fov);
			cam->setNearValue(saved.near_plane);
			cam->setFarValue(saved.far_plane);
			cam->setAspectRatio(saved.aspect);
			cam->updateAbsolutePosition();
		}
	}
#endif

	return Value(true);
}

// RenderTextureObject Implementation
RenderTextureObject::RenderTextureObject() : UnapiObject("render.texture") {
	registerGetter("name", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderTextureObject*>(obj)->m_name);
	});
	registerSetter("name", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderTextureObject*>(obj)->setName(v.asString());
		return true;
	});

	registerGetter("width", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderTextureObject*>(obj)->m_width));
	});
	registerGetter("height", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderTextureObject*>(obj)->m_height));
	});

	registerMethod("get_reference", [](UnapiObject *obj, const ValueArray &args) {
		return Value(static_cast<const RenderTextureObject*>(obj)->m_name);
	});

	registerMethod("update_data", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty() && args[0].type == ValueType::Buffer) {
			auto buf = std::get<std::vector<uint8_t>>(args[0].data);
			static_cast<RenderTextureObject*>(obj)->updateData(buf);
		}
		return Value(true);
	});
}

// RenderTargetObject Implementation
RenderTargetObject::RenderTargetObject() : UnapiObject("render.target") {
	m_texture = std::make_shared<RenderTextureObject>();
	addChild("texture", m_texture);

	m_texture_name = "unapi_rt_" + std::to_string(reinterpret_cast<uintptr_t>(this));
	std::static_pointer_cast<RenderTextureObject>(m_texture)->setName(m_texture_name);

	registerGetter("width", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderTargetObject*>(obj)->m_width));
	});
	registerGetter("height", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderTargetObject*>(obj)->m_height));
	});
	registerGetter("texture", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderTargetObject*>(obj)->m_texture);
	});

	registerMethod("resize", [](UnapiObject *obj, const ValueArray &args) {
		auto tgt = static_cast<RenderTargetObject*>(obj);
		if (args.size() >= 2) {
			tgt->resize(static_cast<int>(args[0].asInt()), static_cast<int>(args[1].asInt()));
		}
		return Value(true);
	});

	registerMethod("bind", [](UnapiObject *obj, const ValueArray &args) {
		return Value(static_cast<RenderTargetObject*>(obj)->bindTarget());
	});

	registerMethod("unbind", [](UnapiObject *obj, const ValueArray &args) {
		return Value(static_cast<RenderTargetObject*>(obj)->unbindTarget());
	});

	registerMethod("get_texture", [](UnapiObject *obj, const ValueArray &args) {
		return Value(static_cast<RenderTargetObject*>(obj)->m_texture);
	});
}

RenderTargetObject::~RenderTargetObject() {
#if IS_CLIENT_BUILD
	auto driver = RenderingEngine::get_video_driver();
	if (driver) {
		if (m_irr_render_target) driver->removeRenderTarget(m_irr_render_target);
		if (m_irr_texture) driver->removeTexture(m_irr_texture);
	}
#endif
}

void RenderTargetObject::resize(int w, int h) {
	if (w <= 0) w = 1;
	if (h <= 0) h = 1;
	m_width = w;
	m_height = h;
	if (m_texture) {
		auto tex = std::static_pointer_cast<RenderTextureObject>(m_texture);
		tex->setDimensions(w, h);
	}
#if IS_CLIENT_BUILD
	auto driver = RenderingEngine::get_video_driver();
	if (driver) {
		if (m_irr_render_target) {
			driver->removeRenderTarget(m_irr_render_target);
			m_irr_render_target = nullptr;
		}
		if (m_irr_texture) {
			driver->removeTexture(m_irr_texture);
			m_irr_texture = nullptr;
		}
	}
#endif
}

bool RenderTargetObject::bindTarget() {
#if IS_CLIENT_BUILD
	auto driver = RenderingEngine::get_video_driver();
	if (driver) {
		if (m_texture_name.empty()) {
			m_texture_name = "unapi_rt_" + std::to_string(getHandleId());
			if (m_texture) {
				std::static_pointer_cast<RenderTextureObject>(m_texture)->setName(m_texture_name);
			}
		}

		if (!m_irr_render_target || !m_irr_texture) {
			m_irr_texture = driver->addRenderTargetTexture(core::dimension2du(m_width, m_height), m_texture_name.c_str(), video::ECF_A8R8G8B8);
			m_irr_render_target = driver->addRenderTarget();
			if (m_irr_render_target && m_irr_texture) {
				m_irr_render_target->setTexture(m_irr_texture, nullptr);
			}
		}

		if (m_irr_render_target) {
			m_saved_render_target = driver->getCurrentRenderTarget();
			m_saved_viewport = driver->getViewPort();

			video::SColor clear_col(255, (u32)(m_clear_color.x * 255), (u32)(m_clear_color.y * 255), (u32)(m_clear_color.z * 255));
			driver->setRenderTargetEx(m_irr_render_target, video::ECBF_COLOR | video::ECBF_DEPTH, clear_col);
			driver->setViewPort(core::rect<s32>(0, 0, m_width, m_height));
		}
	}
#endif
	return true;
}

bool RenderTargetObject::unbindTarget() {
#if IS_CLIENT_BUILD
	auto driver = RenderingEngine::get_video_driver();
	if (driver) {
		driver->setRenderTargetEx(m_saved_render_target, video::ECBF_NONE);
		if (m_saved_viewport.getWidth() > 0 && m_saved_viewport.getHeight() > 0) {
			driver->setViewPort(m_saved_viewport);
		}
	}
#endif
	return true;
}

// RenderPassObject Implementation
RenderPassObject::RenderPassObject() : UnapiObject("render.pass") {
	registerGetter("enabled", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderPassObject*>(obj)->m_enabled);
	});
	registerSetter("enabled", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderPassObject*>(obj)->m_enabled = v.asBool(true);
		return true;
	});

	registerGetter("recursion_depth", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderPassObject*>(obj)->m_recursion_depth));
	});
	registerSetter("recursion_depth", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderPassObject*>(obj)->m_recursion_depth = static_cast<int>(v.asInt());
		return true;
	});

	registerGetter("max_recursion_depth", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderPassObject*>(obj)->m_max_recursion_depth));
	});
	registerSetter("max_recursion_depth", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderPassObject*>(obj)->m_max_recursion_depth = static_cast<int>(v.asInt(3));
		return true;
	});

	registerGetter("camera", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderPassObject*>(obj)->m_camera);
	});
	registerSetter("camera", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderPassObject*>(obj)->m_camera = v.asObject();
		return true;
	});

	registerGetter("target", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderPassObject*>(obj)->m_target);
	});
	registerSetter("target", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderPassObject*>(obj)->m_target = v.asObject();
		return true;
	});

	registerGetter("world", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderPassObject*>(obj)->m_world);
	});
	registerSetter("world", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderPassObject*>(obj)->m_world = v.asObject();
		return true;
	});

	registerMethod("execute", [](UnapiObject *obj, const ValueArray &args) {
		return static_cast<RenderPassObject*>(obj)->executePass();
	});

	registerMethod("set_camera", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty()) static_cast<RenderPassObject*>(obj)->m_camera = args[0].asObject();
		return Value(true);
	});

	registerMethod("set_target", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty()) static_cast<RenderPassObject*>(obj)->m_target = args[0].asObject();
		return Value(true);
	});

	registerMethod("set_world", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty()) static_cast<RenderPassObject*>(obj)->m_world = args[0].asObject();
		return Value(true);
	});
}

Value RenderPassObject::executePass() {
	if (!m_enabled) return Value(false);

	// Protect against infinite recursive rendering loops
	if (m_recursion_depth >= m_max_recursion_depth) {
		infostream << "[UNAPI] RenderPass recursion depth limit reached ("
		           << m_recursion_depth << "/" << m_max_recursion_depth << "). Aborting pass execution." << std::endl;
		return Value(false);
	}

	m_recursion_depth++;

	if (m_target) {
		auto tgt = std::static_pointer_cast<RenderTargetObject>(m_target);
		tgt->bindTarget();
	}

	if (m_camera) {
		auto cam = std::static_pointer_cast<RenderCamera>(m_camera);
		cam->renderView();
	}

	ValueArray pass_args;
	pass_args.push_back(Value(getHandleId()));
	if (m_camera) pass_args.push_back(Value(m_camera));
	if (m_target) pass_args.push_back(Value(m_target));

	Value result = HookRegistry::get().executeHooks("render.pass.execute", pass_args, [](const ValueArray &args) {
#if IS_CLIENT_BUILD
		auto smgr = RenderingEngine::get_raw_device()->getSceneManager();
		if (smgr) {
			smgr->drawAll();
		}
#endif
		return Value(true);
	});

	if (m_target) {
		auto tgt = std::static_pointer_cast<RenderTargetObject>(m_target);
		tgt->unbindTarget();
	}

	m_recursion_depth--;
	return result;
}

// RenderShaderObject Implementation
RenderShaderObject::RenderShaderObject() : UnapiObject("render.shader") {
	registerGetter("name", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderShaderObject*>(obj)->m_name);
	});
	registerSetter("name", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderShaderObject*>(obj)->m_name = v.asString();
		return true;
	});

	registerMethod("bind", [](UnapiObject *obj, const ValueArray &args) {
		static_cast<RenderShaderObject*>(obj)->bindShader();
		return Value(true);
	});

	registerMethod("set_uniform_float", [](UnapiObject *obj, const ValueArray &args) {
		if (args.size() >= 2) {
			static_cast<RenderShaderObject*>(obj)->setUniformFloat(args[0].asString(), static_cast<float>(args[1].asFloat()));
		}
		return Value(true);
	});

	registerMethod("set_uniform_vector", [](UnapiObject *obj, const ValueArray &args) {
		if (args.size() >= 2) {
			static_cast<RenderShaderObject*>(obj)->setUniformVector(args[0].asString(), args[1].asVector3());
		}
		return Value(true);
	});
}

void RenderShaderObject::bindShader() {
	ValueArray args;
	args.push_back(Value(m_name));
	HookRegistry::get().executeHooks("render.shader.bind", args);
}

// RenderMeshObject Implementation
RenderMeshObject::RenderMeshObject() : UnapiObject("render.mesh") {
	registerGetter("vertex_count", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderMeshObject*>(obj)->m_vertex_count));
	});

	registerGetter("index_count", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const RenderMeshObject*>(obj)->m_index_count));
	});
}

// RenderVisualStateObject Implementation
RenderVisualStateObject::RenderVisualStateObject() : UnapiObject("render.visual_state") {
	registerGetter("sky_color", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderVisualStateObject*>(obj)->m_sky_color);
	});
	registerSetter("sky_color", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderVisualStateObject*>(obj)->m_sky_color = v.asVector3();
		return true;
	});

	registerGetter("fog_distance", [](const UnapiObject *obj) {
		return Value(static_cast<const RenderVisualStateObject*>(obj)->m_fog_distance);
	});
	registerSetter("fog_distance", [](UnapiObject *obj, const Value &v) {
		static_cast<RenderVisualStateObject*>(obj)->m_fog_distance = static_cast<float>(v.asFloat(1000.0));
		return true;
	});

	registerMethod("apply", [](UnapiObject *obj, const ValueArray &args) {
		static_cast<RenderVisualStateObject*>(obj)->applyState();
		return Value(true);
	});
}

void RenderVisualStateObject::applyState() {
	ValueArray args;
	args.push_back(Value(m_sky_color));
	args.push_back(Value(m_fog_distance));
	HookRegistry::get().executeHooks("render.visual_state.apply", args);
}

// Gameplay Objects Implementation
GameWorldObject::GameWorldObject() : UnapiObject("game.world") {
	registerGetter("name", [](const UnapiObject *obj) {
		return Value(static_cast<const GameWorldObject*>(obj)->getName());
	});
	registerSetter("name", [](UnapiObject *obj, const Value &v) {
		static_cast<GameWorldObject*>(obj)->setName(v.asString());
		return true;
	});

	registerGetter("instance_id", [](const UnapiObject *obj) {
		return Value(static_cast<int64_t>(static_cast<const GameWorldObject*>(obj)->getInstanceId()));
	});
	registerSetter("instance_id", [](UnapiObject *obj, const Value &v) {
		static_cast<GameWorldObject*>(obj)->setInstanceId(static_cast<int>(v.asInt()));
		return true;
	});

	registerMethod("spawn_entity", [](UnapiObject *obj, const ValueArray &args) {
		std::string type = args.size() >= 1 ? args[0].asString() : "generic";
		Vector3f pos = args.size() >= 2 ? args[1].asVector3() : Vector3f(0, 0, 0);
		return static_cast<GameWorldObject*>(obj)->spawnEntity(type, pos);
	});

	registerMethod("update", [](UnapiObject *obj, const ValueArray &args) {
		ValueArray hook_args;
		hook_args.push_back(Value(obj->getHandleId()));
		if (!args.empty()) hook_args.push_back(args[0]);
		HookRegistry::get().executeHooks("world.update", hook_args);
		return Value(true);
	});

	registerMethod("step_physics", [](UnapiObject *obj, const ValueArray &args) {
		ValueArray hook_args;
		hook_args.push_back(Value(obj->getHandleId()));
		if (!args.empty()) hook_args.push_back(args[0]);
		HookRegistry::get().executeHooks("physics.step", hook_args);
		return Value(true);
	});
}

ObjectRefPtr GameWorldObject::spawnEntity(const std::string &type, Vector3f pos) {
	auto entity = std::make_shared<GameEntityObject>();
	entity->setPosition(pos);
	UnapiRegistry::get().registerObjectInstance(entity);
	m_entities.push_back(entity);
	return entity;
}

GameEntityObject::GameEntityObject() : UnapiObject("game.entity") {
	registerGetter("position", [](const UnapiObject *obj) {
		return Value(static_cast<const GameEntityObject*>(obj)->m_position);
	});
	registerSetter("position", [](UnapiObject *obj, const Value &v) {
		static_cast<GameEntityObject*>(obj)->m_position = v.asVector3();
		return true;
	});

	registerGetter("rotation", [](const UnapiObject *obj) {
		return Value(static_cast<const GameEntityObject*>(obj)->m_rotation);
	});
	registerSetter("rotation", [](UnapiObject *obj, const Value &v) {
		static_cast<GameEntityObject*>(obj)->m_rotation = v.asVector3();
		return true;
	});

	registerGetter("yaw", [](const UnapiObject *obj) {
		return Value(static_cast<const GameEntityObject*>(obj)->m_yaw);
	});
	registerSetter("yaw", [](UnapiObject *obj, const Value &v) {
		static_cast<GameEntityObject*>(obj)->m_yaw = static_cast<float>(v.asFloat());
		return true;
	});

	registerMethod("move", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty()) {
			static_cast<GameEntityObject*>(obj)->move(args[0].asVector3());
		}
		return Value(true);
	});
}

GamePlayerObject::GamePlayerObject() : UnapiObject("game.player") {
	m_camera = std::make_shared<RenderCamera>();
	addChild("camera", m_camera);

	registerGetter("name", [](const UnapiObject *obj) {
		return Value(static_cast<const GamePlayerObject*>(obj)->m_name);
	});
	registerGetter("camera", [](const UnapiObject *obj) {
		return Value(static_cast<const GamePlayerObject*>(obj)->m_camera);
	});
}

GamePhysicsObject::GamePhysicsObject() : UnapiObject("game.physics") {
	registerGetter("gravity", [](const UnapiObject *obj) {
		return Value(static_cast<const GamePhysicsObject*>(obj)->m_gravity);
	});
	registerSetter("gravity", [](UnapiObject *obj, const Value &v) {
		static_cast<GamePhysicsObject*>(obj)->m_gravity = static_cast<float>(v.asFloat(9.81));
		return true;
	});

	registerMethod("step", [](UnapiObject *obj, const ValueArray &args) {
		float dtime = !args.empty() ? static_cast<float>(args[0].asFloat(0.016)) : 0.016f;
		static_cast<GamePhysicsObject*>(obj)->stepPhysics(dtime);
		return Value(true);
	});
}

void GamePhysicsObject::stepPhysics(float dtime) {
	ValueArray args;
	args.push_back(Value(dtime));
	args.push_back(Value(m_gravity));
	HookRegistry::get().executeHooks("physics.step", args);
}

// Asset Objects Implementation
AssetSkeletonObject::AssetSkeletonObject() : UnapiObject("asset.skeleton") {
	registerMethod("get_bone_transform", [](UnapiObject *obj, const ValueArray &args) {
		std::string bone = !args.empty() ? args[0].asString() : "";
		TransformMatrix tf = static_cast<AssetSkeletonObject*>(obj)->getBoneTransform(bone);
		return Value(tf);
	});

	registerMethod("set_bone_transform", [](UnapiObject *obj, const ValueArray &args) {
		if (args.size() >= 2 && args[1].type == ValueType::Transform4) {
			auto tf = std::get<TransformMatrix>(args[1].data);
			static_cast<AssetSkeletonObject*>(obj)->setBoneTransform(args[0].asString(), tf);
		}
		return Value(true);
	});
}

TransformMatrix AssetSkeletonObject::getBoneTransform(const std::string &name) const {
	auto it = m_bone_transforms.find(name);
	if (it != m_bone_transforms.end()) return it->second;
	return TransformMatrix();
}

AssetModelObject::AssetModelObject() : UnapiObject("asset.model") {
	m_skeleton = std::make_shared<AssetSkeletonObject>();
	addChild("skeleton", m_skeleton);

	registerGetter("skeleton", [](const UnapiObject *obj) {
		return Value(static_cast<const AssetModelObject*>(obj)->m_skeleton);
	});

	registerMethod("get_skeleton", [](UnapiObject *obj, const ValueArray &args) {
		return Value(static_cast<AssetModelObject*>(obj)->m_skeleton);
	});
}

AssetAnimationObject::AssetAnimationObject() : UnapiObject("asset.animation") {
	registerGetter("speed", [](const UnapiObject *obj) {
		return Value(static_cast<const AssetAnimationObject*>(obj)->m_speed);
	});
	registerSetter("speed", [](UnapiObject *obj, const Value &v) {
		static_cast<AssetAnimationObject*>(obj)->m_speed = static_cast<float>(v.asFloat(1.0));
		return true;
	});

	registerGetter("playing", [](const UnapiObject *obj) {
		return Value(static_cast<const AssetAnimationObject*>(obj)->isPlaying());
	});

	registerMethod("play", [](UnapiObject *obj, const ValueArray &args) {
		static_cast<AssetAnimationObject*>(obj)->play();
		return Value(true);
	});

	registerMethod("stop", [](UnapiObject *obj, const ValueArray &args) {
		static_cast<AssetAnimationObject*>(obj)->stop();
		return Value(true);
	});
}

AssetMaterialObject::AssetMaterialObject() : UnapiObject("asset.material") {
	registerGetter("texture", [](const UnapiObject *obj) {
		return Value(static_cast<const AssetMaterialObject*>(obj)->getTexture());
	});
	registerSetter("texture", [](UnapiObject *obj, const Value &v) {
		static_cast<AssetMaterialObject*>(obj)->setTexture(v.asObject());
		return true;
	});

	registerGetter("shader", [](const UnapiObject *obj) {
		return Value(static_cast<const AssetMaterialObject*>(obj)->getShader());
	});
	registerSetter("shader", [](UnapiObject *obj, const Value &v) {
		static_cast<AssetMaterialObject*>(obj)->setShader(v.asObject());
		return true;
	});

	registerMethod("set_texture", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty()) {
			static_cast<AssetMaterialObject*>(obj)->setTexture(args[0].asObject());
		}
		return Value(true);
	});

	registerMethod("set_shader", [](UnapiObject *obj, const ValueArray &args) {
		if (!args.empty()) {
			static_cast<AssetMaterialObject*>(obj)->setShader(args[0].asObject());
		}
		return Value(true);
	});
}

AssetTextureObject::AssetTextureObject() : UnapiObject("asset.texture") {
	registerGetter("path", [](const UnapiObject *obj) {
		return Value(static_cast<const AssetTextureObject*>(obj)->m_path);
	});
	registerSetter("path", [](UnapiObject *obj, const Value &v) {
		static_cast<AssetTextureObject*>(obj)->m_path = v.asString();
		return true;
	});
}

void registerAllFoundationTypes() {
	auto &reg = UnapiRegistry::get();

	// Render Foundation Types
	TypeDescriptor camera_desc;
	camera_desc.type_name = "render.camera";
	camera_desc.foundation = "render";
	camera_desc.description = "Independent 3D camera extension object";
	camera_desc.properties["position"] = {"position", ValueType::Vector3, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Camera position"};
	camera_desc.properties["rotation"] = {"rotation", ValueType::Vector3, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Camera rotation"};
	camera_desc.properties["fov"] = {"fov", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Field of view in degrees"};
	camera_desc.properties["near_plane"] = {"near_plane", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Near clipping plane"};
	camera_desc.properties["far_plane"] = {"far_plane", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Far clipping plane"};
	camera_desc.properties["enabled"] = {"enabled", ValueType::Boolean, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Camera enabled flag"};
	camera_desc.properties["world"] = {"world", ValueType::ObjectRef, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "World reference target"};
	reg.registerType(camera_desc, []() { return std::make_shared<RenderCamera>(); });

	TypeDescriptor target_desc;
	target_desc.type_name = "render.target";
	target_desc.foundation = "render";
	target_desc.description = "Offscreen framebuffer render target extension object";
	target_desc.properties["width"] = {"width", ValueType::Integer, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderCreateResources, "Target width"};
	target_desc.properties["height"] = {"height", ValueType::Integer, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderCreateResources, "Target height"};
	target_desc.properties["texture"] = {"texture", ValueType::ObjectRef, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderCreateResources, "Associated render texture"};
	reg.registerType(target_desc, []() { return std::make_shared<RenderTargetObject>(); });

	TypeDescriptor pass_desc;
	pass_desc.type_name = "render.pass";
	pass_desc.foundation = "render";
	pass_desc.description = "Configurable pipeline render pass execution step";
	pass_desc.properties["enabled"] = {"enabled", ValueType::Boolean, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Render pass enabled"};
	pass_desc.properties["recursion_depth"] = {"recursion_depth", ValueType::Integer, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Current pass recursion depth"};
	pass_desc.properties["max_recursion_depth"] = {"max_recursion_depth", ValueType::Integer, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Maximum recursion depth limit"};
	pass_desc.properties["camera"] = {"camera", ValueType::ObjectRef, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Pass camera reference"};
	pass_desc.properties["target"] = {"target", ValueType::ObjectRef, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Pass render target reference"};
	pass_desc.properties["world"] = {"world", ValueType::ObjectRef, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Pass world reference"};
	reg.registerType(pass_desc, []() { return std::make_shared<RenderPassObject>(); });

	TypeDescriptor texture_desc;
	texture_desc.type_name = "render.texture";
	texture_desc.foundation = "render";
	texture_desc.description = "GPU Texture resource";
	texture_desc.properties["name"] = {"name", ValueType::String, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderCreateResources, "Texture name identifier"};
	texture_desc.properties["width"] = {"width", ValueType::Integer, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderCreateResources, "Texture width"};
	texture_desc.properties["height"] = {"height", ValueType::Integer, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderCreateResources, "Texture height"};
	reg.registerType(texture_desc, []() { return std::make_shared<RenderTextureObject>(); });

	TypeDescriptor shader_desc;
	shader_desc.type_name = "render.shader";
	shader_desc.foundation = "render";
	shader_desc.description = "Custom shader program extension object";
	shader_desc.properties["name"] = {"name", ValueType::String, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Shader identifier"};
	reg.registerType(shader_desc, []() { return std::make_shared<RenderShaderObject>(); });

	TypeDescriptor mesh_desc;
	mesh_desc.type_name = "render.mesh";
	mesh_desc.foundation = "render";
	mesh_desc.description = "Visual mesh geometry extension object";
	mesh_desc.properties["vertex_count"] = {"vertex_count", ValueType::Integer, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderAccess, "Number of vertices"};
	mesh_desc.properties["index_count"] = {"index_count", ValueType::Integer, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderAccess, "Number of indices"};
	reg.registerType(mesh_desc, []() { return std::make_shared<RenderMeshObject>(); });

	TypeDescriptor vstate_desc;
	vstate_desc.type_name = "render.visual_state";
	vstate_desc.foundation = "render";
	vstate_desc.description = "Visual rendering state override object";
	vstate_desc.properties["sky_color"] = {"sky_color", ValueType::Vector3, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Sky RGB color"};
	vstate_desc.properties["fog_distance"] = {"fog_distance", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Fog distance in units"};
	reg.registerType(vstate_desc, []() { return std::make_shared<RenderVisualStateObject>(); });

	// Gameplay Foundation Types
	TypeDescriptor world_desc;
	world_desc.type_name = "game.world";
	world_desc.foundation = "game";
	world_desc.description = "Game world or dimension instance extension object";
	world_desc.properties["name"] = {"name", ValueType::String, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::WorldModify, "World name"};
	world_desc.properties["instance_id"] = {"instance_id", ValueType::Integer, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::WorldModify, "Dimension/World instance ID"};
	reg.registerType(world_desc, []() { return std::make_shared<GameWorldObject>(); });

	TypeDescriptor entity_desc;
	entity_desc.type_name = "game.entity";
	entity_desc.foundation = "game";
	entity_desc.description = "Active entity instance extension object";
	entity_desc.properties["position"] = {"position", ValueType::Vector3, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::WorldModify, "Entity position"};
	entity_desc.properties["rotation"] = {"rotation", ValueType::Vector3, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::WorldModify, "Entity rotation"};
	entity_desc.properties["yaw"] = {"yaw", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::WorldModify, "Entity yaw in degrees"};
	reg.registerType(entity_desc, []() { return std::make_shared<GameEntityObject>(); });

	TypeDescriptor player_desc;
	player_desc.type_name = "game.player";
	player_desc.foundation = "game";
	player_desc.description = "Local or remote player extension object";
	player_desc.properties["name"] = {"name", ValueType::String, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::WorldModify, "Player username"};
	player_desc.properties["camera"] = {"camera", ValueType::ObjectRef, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderAccess, "Player main camera reference"};
	reg.registerType(player_desc, []() { return std::make_shared<GamePlayerObject>(); });

	TypeDescriptor physics_desc;
	physics_desc.type_name = "game.physics";
	physics_desc.foundation = "game";
	physics_desc.description = "Physics engine state extension object";
	physics_desc.properties["gravity"] = {"gravity", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::PhysicsModify, "World gravity value"};
	reg.registerType(physics_desc, []() { return std::make_shared<GamePhysicsObject>(); });

	// Asset Foundation Types
	TypeDescriptor model_desc;
	model_desc.type_name = "asset.model";
	model_desc.foundation = "asset";
	model_desc.description = "3D Asset model resource object";
	model_desc.properties["skeleton"] = {"skeleton", ValueType::ObjectRef, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderAccess, "Model skeleton reference"};
	reg.registerType(model_desc, []() { return std::make_shared<AssetModelObject>(); });

	TypeDescriptor skel_desc;
	skel_desc.type_name = "asset.skeleton";
	skel_desc.foundation = "asset";
	skel_desc.description = "Skeleton bone structure resource object";
	reg.registerType(skel_desc, []() { return std::make_shared<AssetSkeletonObject>(); });

	TypeDescriptor anim_desc;
	anim_desc.type_name = "asset.animation";
	anim_desc.foundation = "asset";
	anim_desc.description = "Animation resource object";
	anim_desc.properties["speed"] = {"speed", ValueType::Float, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Animation speed multiplier"};
	anim_desc.properties["playing"] = {"playing", ValueType::Boolean, PropertyAccess::ReadOnly, ApiStability::Stable, Permission::RenderAccess, "Is animation playing"};
	reg.registerType(anim_desc, []() { return std::make_shared<AssetAnimationObject>(); });

	TypeDescriptor mat_desc;
	mat_desc.type_name = "asset.material";
	mat_desc.foundation = "asset";
	mat_desc.description = "Visual material resource object";
	mat_desc.properties["texture"] = {"texture", ValueType::ObjectRef, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Material texture reference"};
	mat_desc.properties["shader"] = {"shader", ValueType::ObjectRef, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Material shader reference"};
	reg.registerType(mat_desc, []() { return std::make_shared<AssetMaterialObject>(); });

	TypeDescriptor atex_desc;
	atex_desc.type_name = "asset.texture";
	atex_desc.foundation = "asset";
	atex_desc.description = "Asset texture image resource";
	atex_desc.properties["path"] = {"path", ValueType::String, PropertyAccess::ReadWrite, ApiStability::Stable, Permission::RenderAccess, "Texture file path"};
	reg.registerType(atex_desc, []() { return std::make_shared<AssetTextureObject>(); });
}

} // namespace unapi
