// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef __ANDROID__

#include "htmlview_jni.h"

#include "config.h"
#include "log.h"
#include "porting_android.h"

#include <jni.h>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "irrlichttypes_bloated.h"
#include "porting.h"
#include "client/renderingengine.h"
#include "client/camera.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "constants.h"
#include "util/base64.h"
#include "util/png.h"
#include <IImage.h>
#include <ICameraSceneNode.h>
#include <IVideoDriver.h>
#include <IWriteFile.h>
#include <IFileSystem.h>
#include <ISceneManager.h>
#include <ISceneCollisionManager.h>
#include <IBillboardSceneNode.h>
#include <IMeshSceneNode.h>
#include <IMesh.h>
#include <IGeometryCreator.h>
#include <ISceneNode.h>
#include <SViewFrustum.h>
#include "irr_ptr.h"

struct HtmlViewAnchor {
	std::string type;
	v3f pos;
	u16 object_id;
	v3f offset;
	v2f size;
	v3f rotation;
	std::string viewport_name;
	scene::ISceneNode *node = nullptr;
	video::ITexture *texture = nullptr;
	video::IImage *image = nullptr;
	std::mutex image_mutex;
	bool image_dirty = false;
	u32 last_image_hash = 0;
	u64 last_request_ms = 0;
	bool active = true;
	bool pending_removal = false;
};

static std::mutex g_anchor_mutex;
static std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<HtmlViewAnchor>>> g_anchors;

struct Viewport {
	v3f pos;
	v3f dir;
	v3f up = v3f(0, 1, 0);
	float fov;
	float tilt = 0.0f;
	int width;
	int height;

	v3f target_pos;
	v3f target_dir;
	bool smooth_pos = false;
	bool smooth_rot = false;
	float pos_smoothing = 0.15f;
	float rot_smoothing = 0.10f;
	std::string update_mode = "continuous";

	u32 refresh_interval_ms = 50; // default 20 fps
	u64 last_render_ms = 0;
	bool dirty = true;
	std::string format = "jpeg";
	int quality = 70;

	video::ITexture *texture = nullptr;
	video::IImage *image = nullptr;
	scene::ICameraSceneNode *camera_node = nullptr;
	std::mutex image_mutex;
	bool active = true;
	bool pending_removal = false;
};

static std::mutex g_viewport_mutex;
static std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Viewport>>> g_viewports;

struct HtmlViewMessage {
	std::string id;
	std::string message;
};

struct HtmlViewCapture {
	std::string id;
	std::string png_base64;
};

struct HtmlViewEvent {
	enum Type { READY } type;
	std::string id;
};

static std::mutex g_msg_mutex;
static std::deque<HtmlViewMessage> g_messages;
static std::deque<HtmlViewCapture> g_captures;
static std::deque<HtmlViewEvent> g_events;

static std::string readJavaString(JNIEnv *env, jstring j_str)
{
	if (!j_str)
		return "";
	const char *c_str = env->GetStringUTFChars(j_str, nullptr);
	if (!c_str) {
		if (env->ExceptionCheck())
			env->ExceptionClear();
		return "";
	}
	std::string str(c_str);
	env->ReleaseStringUTFChars(j_str, c_str);
	return str;
}

static void callVoidMethod2Str(const char *method_name, const std::string &a, const std::string &b)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name,
		"(Ljava/lang/String;Ljava/lang/String;)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return;
	}

	jstring ja = env->NewStringUTF(a.c_str());
	jstring jb = env->NewStringUTF(b.c_str());
	env->CallVoidMethod(porting::activity, mid, ja, jb);
	if (ja)
		env->DeleteLocalRef(ja);
	if (jb)
		env->DeleteLocalRef(jb);
}

static void callVoidMethod3Str(const char *method_name, const std::string &a,
		const std::string &b, const std::string &c)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name,
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return;
	}

	jstring ja = env->NewStringUTF(a.c_str());
	jstring jb = env->NewStringUTF(b.c_str());
	jstring jc = env->NewStringUTF(c.c_str());
	env->CallVoidMethod(porting::activity, mid, ja, jb, jc);
	if (ja)
		env->DeleteLocalRef(ja);
	if (jb)
		env->DeleteLocalRef(jb);
	if (jc)
		env->DeleteLocalRef(jc);
}

static void callVoidMethod1Str(const char *method_name, const std::string &a)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name,
		"(Ljava/lang/String;)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return;
	}

	jstring ja = env->NewStringUTF(a.c_str());
	env->CallVoidMethod(porting::activity, mid, ja);
	if (ja)
		env->DeleteLocalRef(ja);
}

static void callVoidMethod1Str2Int(const char *method_name, const std::string &a,
			int b, int c)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name,
		"(Ljava/lang/String;II)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return;
	}

	jstring ja = env->NewStringUTF(a.c_str());
	jint jb = b;
	jint jc = c;
	env->CallVoidMethod(porting::activity, mid, ja, jb, jc);
	if (ja)
		env->DeleteLocalRef(ja);
}

static void callVoidMethod0(const char *method_name)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name, "()V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return;
	}

	env->CallVoidMethod(porting::activity, mid);
}

static void callVoidMethod1Str1Bool(const char *method_name, const std::string &a, bool b)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name, "(Ljava/lang/String;Z)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return;
	}

	jstring ja = env->NewStringUTF(a.c_str());
	jboolean jb = b;
	env->CallVoidMethod(porting::activity, mid, ja, jb);
	if (ja)
		env->DeleteLocalRef(ja);
}

static std::string callStringMethod1Str(const char *method_name, const std::string &a)
{
	JNIEnv *env = porting::getJNIEnv();
	jmethodID mid = env->GetMethodID(porting::activityClass, method_name,
		"(Ljava/lang/String;)Ljava/lang/String;");
	if (!mid) {
		errorstream << "htmlview_jni: missing method " << method_name << std::endl;
		return "";
	}

	jstring ja = env->NewStringUTF(a.c_str());
	jstring jr = (jstring)env->CallObjectMethod(porting::activity, mid, ja);
	if (ja)
		env->DeleteLocalRef(ja);
	std::string r = readJavaString(env, jr);
	if (jr)
		env->DeleteLocalRef(jr);
	return r;
}

void htmlview_jni_run(const std::string &id, const std::string &html)
{
	callVoidMethod2Str("htmlview_run", id, html);
}

void htmlview_jni_run_worker(const std::string &id, const std::string &html)
{
	callVoidMethod2Str("htmlview_run_worker", id, html);
}

void htmlview_jni_run_external(const std::string &id, const std::string &root_dir,
		const std::string &entry)
{
	callVoidMethod3Str("htmlview_run_external", id, root_dir, entry);
}

void htmlview_jni_run_external_worker(const std::string &id, const std::string &root_dir,
		const std::string &entry)
{
	callVoidMethod3Str("htmlview_run_external_worker", id, root_dir, entry);
}

void htmlview_jni_stop(const std::string &id)
{
	callVoidMethod1Str("htmlview_stop", id);
	{
		std::lock_guard<std::mutex> lock(g_anchor_mutex);
		auto it = g_anchors.find(id);
		if (it != g_anchors.end()) {
			for (auto &pair : it->second) {
				pair.second->pending_removal = true;
			}
		}
	}
	{
		std::lock_guard<std::mutex> lock(g_viewport_mutex);
		auto it = g_viewports.find(id);
		if (it != g_viewports.end()) {
			for (auto &pair : it->second) {
				pair.second->pending_removal = true;
			}
		}
	}
}

void htmlview_jni_shutdown_all()
{
	callVoidMethod0("htmlview_shutdown_all");
	{
		std::lock_guard<std::mutex> lock(g_anchor_mutex);
		for (auto &pair : g_anchors) {
			for (auto &pair2 : pair.second) {
				pair2.second->pending_removal = true;
			}
		}
	}
	{
		std::lock_guard<std::mutex> lock(g_viewport_mutex);
		for (auto &pair : g_viewports) {
			for (auto &pair2 : pair.second) {
				pair2.second->pending_removal = true;
			}
		}
	}
}

void htmlview_jni_reload(const std::string &id)
{
	callVoidMethod1Str("htmlview_reload", id);
}

void htmlview_jni_focus(const std::string &id)
{
	callVoidMethod1Str("htmlview_focus", id);
}

std::string htmlview_jni_state(const std::string &id)
{
	return callStringMethod1Str("htmlview_state", id);
}

void htmlview_jni_display(const std::string &id, int x, int y, int w, int h,
		bool visible, bool fullscreen, bool safe_area,
		bool drag_embed, float border_radius)
{
	JNIEnv *env = porting::getJNIEnv();

	jmethodID mid = env->GetMethodID(porting::activityClass, "htmlview_display",
		"(Ljava/lang/String;IIIIZZZZF)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method htmlview_display" << std::endl;
		return;
	}

	jstring jid = env->NewStringUTF(id.c_str());
	jint jx = x;
	jint jy = y;
	jint jw = w;
	jint jh = h;
	jboolean jvis = visible;
	jboolean jfull = fullscreen;
	jboolean jsafe = safe_area;
	jboolean jdrag = drag_embed;
	jfloat jrad = border_radius;
	
	env->CallVoidMethod(porting::activity, mid, jid, jx, jy, jw, jh, jvis, jfull, jsafe, jdrag, jrad);
	if (jid)
		env->DeleteLocalRef(jid);
}

void htmlview_jni_input(const std::string &id, bool block_game_input)
{
	callVoidMethod1Str1Bool("htmlview_input", id, block_game_input);
}

void htmlview_jni_send(const std::string &id, const std::string &message)
{
	callVoidMethod2Str("htmlview_send", id, message);
}

void htmlview_jni_navigate(const std::string &id, const std::string &url)
{
	callVoidMethod2Str("htmlview_navigate", id, url);
}

void htmlview_jni_inject(const std::string &id, const std::string &js)
{
	callVoidMethod2Str("htmlview_inject", id, js);
}

void htmlview_jni_pipe(const std::string &fromId, const std::string &toId)
{
	callVoidMethod2Str("htmlview_pipe", fromId, toId);
}

void htmlview_jni_shared_set(const std::string &key, const char *val)
{
	JNIEnv *env = porting::getJNIEnv();

	jmethodID mid = env->GetMethodID(porting::activityClass, "htmlview_shared_set",
		"(Ljava/lang/String;Ljava/lang/String;)V");
	if (!mid) {
		errorstream << "htmlview_jni: missing method htmlview_shared_set" << std::endl;
		return;
	}

	jstring jk = env->NewStringUTF(key.c_str());
	jstring jv = val ? env->NewStringUTF(val) : nullptr;
	env->CallVoidMethod(porting::activity, mid, jk, jv);
	if (jk)
		env->DeleteLocalRef(jk);
	if (jv)
		env->DeleteLocalRef(jv);
}

std::string htmlview_jni_shared_get(const std::string &key)
{
	return callStringMethod1Str("htmlview_shared_get", key);
}

void htmlview_jni_capture(const std::string &id, int width, int height)
{
	callVoidMethod1Str2Int("htmlview_capture", id, width, height);
}

void htmlview_jni_set_viewport(const std::string &id, const std::string &name,
		v3f pos, v3f dir, v3f up, float fov, float tilt, int width, int height,
		u32 refresh_interval_ms, const std::string &format, int quality,
		bool smooth_pos, bool smooth_rot, float pos_smooth, float rot_smooth,
		const std::string &update_mode)
{
	std::lock_guard<std::mutex> lock(g_viewport_mutex);
	auto &vps = g_viewports[id];
	auto it = vps.find(name);
	if (it == vps.end()) {
		auto vp = std::make_unique<Viewport>();
		vp->pos = pos;
		vp->dir = dir;
		vp->up = up;
		vp->fov = fov;
		vp->tilt = tilt;
		vp->width = width;
		vp->height = height;
		vp->target_pos = pos;
		vp->target_dir = dir;
		vp->smooth_pos = smooth_pos;
		vp->smooth_rot = smooth_rot;
		vp->pos_smoothing = pos_smooth;
		vp->rot_smoothing = rot_smooth;
		vp->update_mode = update_mode;
		vp->refresh_interval_ms = refresh_interval_ms;
		vp->format = format;
		vp->quality = quality;
		vp->dirty = true;
		vps[name] = std::move(vp);
	} else {
		auto &vp = it->second;
		if (vp->fov != fov || vp->width != width || vp->height != height ||
				vp->format != format || vp->quality != quality || vp->update_mode != update_mode) {
			vp->dirty = true;
		}
		if (!smooth_pos) vp->pos = pos;
		if (!smooth_rot) vp->dir = dir;
		vp->target_pos = pos;
		vp->target_dir = dir;
		vp->up = up;
		vp->fov = fov;
		vp->tilt = tilt;
		vp->width = width;
		vp->height = height;
		vp->smooth_pos = smooth_pos;
		vp->smooth_rot = smooth_rot;
		vp->pos_smoothing = pos_smooth;
		vp->rot_smoothing = rot_smooth;
		vp->update_mode = update_mode;
		vp->refresh_interval_ms = refresh_interval_ms;
		vp->format = format;
		vp->quality = quality;
		vp->active = true;
		vp->pending_removal = false;
	}
}

void htmlview_jni_remove_viewport(const std::string &id, const std::string &name)
{
	std::lock_guard<std::mutex> lock(g_viewport_mutex);
	auto it = g_viewports.find(id);
	if (it != g_viewports.end()) {
		auto it2 = it->second.find(name);
		if (it2 != it->second.end()) {
			it2->second->pending_removal = true;
		}
	}
}

void htmlview_jni_set_anchor(const std::string &id, const std::string &name,
		const std::string &type, v3f pos, u16 object_id, v3f offset,
		v2f size, v3f rotation, const std::string &viewport_name)
{
	std::lock_guard<std::mutex> lock(g_anchor_mutex);
	auto &instances = g_anchors[id];
	auto it = instances.find(name);

	if (type.empty()) {
		if (it != instances.end())
			it->second->pending_removal = true;
		return;
	}

	auto &anchor = instances[name];
	if (!anchor)
		anchor = std::make_unique<HtmlViewAnchor>();

	if (anchor->node && anchor->type != type) {
		anchor->node->remove();
		anchor->node = nullptr;
	}

	anchor->type = type;
	anchor->pos = pos;
	anchor->object_id = object_id;
	anchor->offset = offset;
	anchor->size = size;
	anchor->rotation = rotation;
	anchor->viewport_name = viewport_name;
	anchor->active = true;
	anchor->pending_removal = false;
}

void htmlview_jni_remove_anchor(const std::string &id, const std::string &name)
{
	std::lock_guard<std::mutex> lock(g_anchor_mutex);
	auto it = g_anchors.find(id);
	if (it != g_anchors.end()) {
		auto it2 = it->second.find(name);
		if (it2 != it->second.end()) {
			it2->second->pending_removal = true;
		}
	}
}

static void htmlview_jni_update_anchors(Client *client, float dtime)
{
	auto driver = RenderingEngine::get_video_driver();
	auto smgr = RenderingEngine::get_raw_device()->getSceneManager();
	auto coll = smgr->getSceneCollisionManager();
	auto camera = smgr->getActiveCamera();

	// Note: Locks (g_viewport_mutex, g_anchor_mutex) must be held by caller
	// in that specific order to avoid deadlocks.
	for (auto &pair : g_anchors) {
		const std::string &id = pair.first;
		auto &instances = pair.second;

		for (auto it = instances.begin(); it != instances.end(); ) {
			auto &anchor = it->second;

			if (anchor->pending_removal) {
				if (anchor->node)
					anchor->node->remove();
				if (anchor->texture)
					driver->removeTexture(anchor->texture);
				{
					std::lock_guard<std::mutex> img_lock(anchor->image_mutex);
					if (anchor->image)
						anchor->image->drop();
				}
				it = instances.erase(it);
				continue;
			}

			v3f world_pos = anchor->pos;
			if (anchor->object_id != 0) {
				auto obj = client->getEnv().getActiveObject(anchor->object_id);
				if (obj) {
					world_pos = obj->getPosition();
				}
			}
			world_pos += anchor->offset;

			if (anchor->type == "2d") {
				core::position2d<s32> screen_pos =
						coll->getScreenCoordinatesFrom3DPosition(world_pos, camera);

				// check if behind camera
				core::matrix4 trans = camera->getProjectionMatrix();
				trans *= camera->getViewMatrix();
				f32 w = trans[3] * world_pos.X + trans[7] * world_pos.Y + trans[11] * world_pos.Z +
						trans[15];
				bool visible = w > 0;

				// Call JNI to reposition
				JNIEnv *env = porting::getJNIEnv();
				jmethodID mid = env->GetMethodID(porting::activityClass, "htmlview_reposition",
						"(Ljava/lang/String;IIZ)V");
				if (mid) {
					jstring jid = env->NewStringUTF(id.c_str());
					env->CallVoidMethod(porting::activity, mid, jid, (jint)screen_pos.X,
							(jint)screen_pos.Y, (jboolean)visible);
					env->DeleteLocalRef(jid);
				}
			} else if (anchor->type == "3d" || anchor->type == "plane") {
				if (!anchor->node) {
					if (anchor->type == "3d") {
						anchor->node = smgr->addBillboardSceneNode(nullptr,
								core::dimension2d<f32>(anchor->size.X, anchor->size.Y));
					} else {
						scene::IMesh *mesh = smgr->getGeometryCreator()->createPlaneMesh(
								core::dimension2d<f32>(anchor->size.X, anchor->size.Y),
								core::dimension2d<u32>(1, 1));
						anchor->node = smgr->addMeshSceneNode(mesh);
						mesh->drop();
					}
					anchor->node->setMaterialFlag(video::EMF_LIGHTING, false);
					anchor->node->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);
				}
				anchor->node->setPosition(world_pos);
				if (anchor->type == "3d") {
					static_cast<scene::IBillboardSceneNode *>(anchor->node)
							->setSize(core::dimension2d<f32>(anchor->size.X, anchor->size.Y));
				} else {
					anchor->node->setRotation(anchor->rotation);
				}

				if (!anchor->viewport_name.empty()) {
					// Use viewport texture
					auto it_vps = g_viewports.find(id);
					if (it_vps != g_viewports.end()) {
						auto it_vp = it_vps->second.find(anchor->viewport_name);
						if (it_vp != it_vps->second.end()) {
							if (it_vp->second->texture) {
								anchor->node->setMaterialTexture(0, it_vp->second->texture);
							}
						}
					}
				} else {
					// Use WebView texture
					// Frustum culling for texture updates
					bool in_frustum = true;
					if (anchor->node) {
						core::aabbox3df box = anchor->node->getBoundingBox();
						anchor->node->getAbsoluteTransformation().transformBoxEx(box);
						if (!camera->getViewFrustum()->getBoundingBox().intersectsWithBox(box)) {
							in_frustum = false;
						}
					}

					u64 now = porting::getTimeMs();
					if (in_frustum && now - anchor->last_request_ms > 33) {
						anchor->last_request_ms = now;
						callVoidMethod1Str("htmlview_request_texture_update", id);
					}

					std::lock_guard<std::mutex> img_lock(anchor->image_mutex);
					if (anchor->image_dirty && anchor->image) {
						core::dimension2du size = anchor->image->getDimension();
						if (!anchor->texture || anchor->texture->getSize() != size) {
							if (anchor->texture)
								driver->removeTexture(anchor->texture);
							static int tex_id = 0;
							std::string tex_name = "htmlview_anchor_" + std::to_string(tex_id++);
							anchor->texture = driver->addTexture(tex_name.c_str(), anchor->image);
						} else {
							void *data = anchor->texture->lock();
							if (data) {
								memcpy(data, anchor->image->getData(),
										size.Width * size.Height * 4);
								anchor->texture->unlock();
							}
						}
						anchor->node->setMaterialTexture(0, anchor->texture);
						anchor->image_dirty = false;
					}
				}
			}
			++it;
		}
	}
}

void htmlview_jni_render_viewports(Client *client, float dtime)
{
	std::lock_guard<std::mutex> vp_lock(g_viewport_mutex);
	std::lock_guard<std::mutex> anchor_lock(g_anchor_mutex);

	htmlview_jni_update_anchors(client, dtime);

	auto driver = RenderingEngine::get_video_driver();
	auto smgr = RenderingEngine::get_raw_device()->getSceneManager();

	for (auto &pair : g_viewports) {
		auto &vps = pair.second;
		for (auto it = vps.begin(); it != vps.end(); ) {
			auto &vp = it->second;
			if (vp->pending_removal) {
				// Clear texture from any anchor referencing this viewport
				auto it_ans = g_anchors.find(pair.first);
				if (it_ans != g_anchors.end()) {
					for (auto &pair_an : it_ans->second) {
						if (pair_an.second->viewport_name == it->first) {
							if (pair_an.second->node)
								pair_an.second->node->setMaterialTexture(0, nullptr);
							pair_an.second->viewport_name = "";
						}
					}
				}

				if (vp->texture)
					driver->removeTexture(vp->texture);
				if (vp->camera_node) {
					vp->camera_node->remove();
					vp->camera_node = nullptr;
				}
				{
					std::lock_guard<std::mutex> img_lock(vp->image_mutex);
					if (vp->image)
						vp->image->drop();
				}
				it = vps.erase(it);
				continue;
			}

			// Apply smoothing
			if (vp->smooth_pos) {
				float factor = 1.0f - std::exp(-dtime / std::max(0.001f, vp->pos_smoothing));
				v3f delta = vp->target_pos - vp->pos;
				if (delta.getLengthSQ() > 0.0001f) {
					vp->pos += delta * factor;
					if (vp->update_mode == "on_change") vp->dirty = true;
				} else {
					vp->pos = vp->target_pos;
				}
			}
			if (vp->smooth_rot) {
				float factor = 1.0f - std::exp(-dtime / std::max(0.001f, vp->rot_smoothing));
				v3f delta = vp->target_dir - vp->dir;
				if (delta.getLengthSQ() > 0.0001f) {
					vp->dir += delta * factor;
					vp->dir.normalize();
					if (vp->update_mode == "on_change") vp->dirty = true;
				} else {
					vp->dir = vp->target_dir;
				}
			}

			u64 now = porting::getTimeMs();
			bool should_render = vp->active;
			if (vp->update_mode == "manual") {
				should_render = should_render && vp->dirty;
			} else if (vp->update_mode == "on_change") {
				should_render = should_render && vp->dirty;
			} else { // continuous
				should_render = should_render && (vp->dirty || (vp->refresh_interval_ms > 0 &&
						now - vp->last_render_ms >= vp->refresh_interval_ms));
			}

			if (should_render) {
				vp->dirty = false;
				vp->last_render_ms = now;

				// Initialize or resize texture
				core::dimension2du size(vp->width, vp->height);
				if (!vp->texture || vp->texture->getSize() != size) {
					if (vp->texture)
						driver->removeTexture(vp->texture);
					static int tex_id = 0;
					std::string tex_name = "htmlview_vp_" + std::to_string(tex_id++);
					vp->texture = driver->addRenderTargetTexture(size, tex_name.c_str(), video::ECF_A8R8G8B8);
				}

				if (vp->texture) {
					if (!vp->camera_node) {
						vp->camera_node = smgr->addCameraSceneNode();
					}

					auto old_cam_node = smgr->getActiveCamera();
					smgr->setActiveCamera(vp->camera_node);

					// Setup viewport camera
					vp->camera_node->setPosition(vp->pos);
					vp->camera_node->setTarget(vp->pos + vp->dir * 100.0f);

					v3f up = vp->up;
					if (vp->tilt != 0.0f) {
						core::quaternion q;
						q.fromAngleAxis(vp->tilt * core::DEGTORAD, vp->dir);
						up = q * up;
					}
					vp->camera_node->setUpVector(up);

					vp->camera_node->setFOV(vp->fov * core::DEGTORAD);
					vp->camera_node->setAspectRatio((float)vp->width / vp->height);
					vp->camera_node->updateMatrices();

					// Render to texture
					driver->setRenderTarget(vp->texture, true, true, video::SColor(255, 0, 0, 0));
					smgr->drawAll();

					// Capture to IImage
					video::IImage *img = driver->createImage(vp->texture, core::position2d<s32>(0, 0), size);
					if (img) {
						std::lock_guard<std::mutex> img_lock(vp->image_mutex);
						if (vp->image)
							vp->image->drop();
						vp->image = img;
					}

					driver->setRenderTarget(0, false, false);
					smgr->setActiveCamera(old_cam_node);
				}
			}
			++it;
		}
	}
}

#if 0
void htmlview_jni_inject(const std::string &id, const std::string &js)
{
	callVoidMethod2Str("htmlview_inject", id, js);
}

void htmlview_jni_pipe(const std::string &fromId, const std::string &toId)
{
	callVoidMethod2Str("htmlview_pipe", fromId, toId);
}
#endif


extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_HTMLViewManager_nativeOnHTMLMessage(
		JNIEnv *env, jclass, jstring id, jstring message)
{
	HtmlViewMessage m;
	m.id = readJavaString(env, id);
	m.message = readJavaString(env, message);
	{
		std::lock_guard<std::mutex> lock(g_msg_mutex);
		g_messages.push_back(std::move(m));
	}
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_HTMLViewManager_nativeOnHTMLCapture(
		JNIEnv *env, jclass, jstring id, jstring png_base64)
{
	HtmlViewCapture c;
	c.id = readJavaString(env, id);
	c.png_base64 = readJavaString(env, png_base64);
	{
		std::lock_guard<std::mutex> lock(g_msg_mutex);
		g_captures.push_back(std::move(c));
	}
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_HTMLViewManager_nativeOnHTMLReady(
		JNIEnv *env, jclass, jstring id)
{
	HtmlViewEvent e;
	e.type = HtmlViewEvent::READY;
	e.id = readJavaString(env, id);
	{
		std::lock_guard<std::mutex> lock(g_msg_mutex);
		g_events.push_back(std::move(e));
	}
}

static u32 quick_hash(const void *data, size_t size)
{
	u32 hash = 5381;
	const u8 *p = (const u8 *)data;
	for (size_t i = 0; i < size; i++)
		hash = ((hash << 5) + hash) + p[i];
	return hash;
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_HTMLViewManager_nativeOnHTMLTextureUpdate(
		JNIEnv *env, jclass, jstring id, jobject byteBuffer, jint w, jint h)
{
	std::string sid = readJavaString(env, id);
	void *data = env->GetDirectBufferAddress(byteBuffer);
	if (!data)
		return;

	std::lock_guard<std::mutex> lock(g_anchor_mutex);
	auto it = g_anchors.find(sid);
	if (it != g_anchors.end()) {
		for (auto &pair : it->second) {
			auto &anchor = pair.second;
			if (!anchor->viewport_name.empty()) continue; // Skip viewport anchors

			u32 hsh = quick_hash(data, w * h * 4);
			if (hsh == anchor->last_image_hash)
				continue;
			anchor->last_image_hash = hsh;

			std::lock_guard<std::mutex> img_lock(anchor->image_mutex);
			core::dimension2du size(w, h);
			if (!anchor->image || anchor->image->getDimension() != size) {
				if (anchor->image)
					anchor->image->drop();
				auto driver = RenderingEngine::get_video_driver();
				anchor->image = driver->createImage(video::ECF_A8R8G8B8, size);
			}
			if (anchor->image) {
				memcpy(anchor->image->getData(), data, w * h * 4);
				anchor->image_dirty = true;
			}
		}
	}
}

bool htmlview_jni_get_viewport(const std::string &id, const std::string &name,
		v3f &pos, v3f &dir, v3f &up, float &fov, float &tilt, int &width, int &height,
		u32 &refresh_interval_ms, std::string &format, int &quality,
		bool &smooth_pos, bool &smooth_rot, float &pos_smooth, float &rot_smooth,
		std::string &update_mode)
{
	std::lock_guard<std::mutex> lock(g_viewport_mutex);
	auto it = g_viewports.find(id);
	if (it != g_viewports.end()) {
		auto it2 = it->second.find(name);
		if (it2 != it->second.end()) {
			auto &vp = it2->second;
			pos = vp->target_pos;
			dir = vp->target_dir;
			up = vp->up;
			fov = vp->fov;
			tilt = vp->tilt;
			width = vp->width;
			height = vp->height;
			refresh_interval_ms = vp->refresh_interval_ms;
			format = vp->format;
			quality = vp->quality;
			smooth_pos = vp->smooth_pos;
			smooth_rot = vp->smooth_rot;
			pos_smooth = vp->pos_smoothing;
			rot_smooth = vp->rot_smoothing;
			update_mode = vp->update_mode;
			return true;
		}
	}
	return false;
}

static std::string encode_image_to_png(video::IImage *image)
{
	auto size = image->getDimension();
	u32 pixel_count = size.Width * size.Height;
	std::vector<u8> rgba_data(pixel_count * 4);
	u8 *src = (u8 *)image->getData();
	for (u32 i = 0; i < pixel_count; ++i) {
		rgba_data[i * 4 + 0] = src[i * 4 + 2]; // R
		rgba_data[i * 4 + 1] = src[i * 4 + 1]; // G
		rgba_data[i * 4 + 2] = src[i * 4 + 0]; // B
		rgba_data[i * 4 + 3] = src[i * 4 + 3]; // A
	}
	return encodePNG(rgba_data.data(), size.Width, size.Height, 6);
}

static std::string encode_viewport_frame(Viewport *vp)
{
	std::lock_guard<std::mutex> img_lock(vp->image_mutex);
	if (!vp->image)
		return "";

	if (vp->format == "png") {
		return encode_image_to_png(vp->image);
	}

	// Use Irrlicht's JPEG writer
	auto driver = RenderingEngine::get_video_driver();
	auto fs = RenderingEngine::get_raw_device()->getFileSystem();

	// Max size for the buffer: width * height * 3 (uncompressed) should be enough for JPEG
	u32 max_size = vp->width * vp->height * 3 + 1024;
	std::vector<u8> buffer(max_size);

	std::string out;
	io::IWriteFile *mem_file =
			fs->createMemoryWriteFile(buffer.data(), max_size, "temp.jpg", false);
	if (mem_file) {
		if (driver->writeImageToFile(vp->image, mem_file, vp->quality)) {
			out.assign((const char *)buffer.data(), mem_file->getPos());
		}
		mem_file->drop();
	}

	if (out.empty()) {
		// Fallback to PNG if JPEG failed
		return encode_image_to_png(vp->image);
	}
	return out;
}

std::string htmlview_jni_get_viewport_frame(const std::string &id, const std::string &name)
{
	std::lock_guard<std::mutex> lock(g_viewport_mutex);
	auto it = g_viewports.find(id);
	if (it != g_viewports.end()) {
		auto it2 = it->second.find(name);
		if (it2 != it->second.end()) {
			return encode_viewport_frame(it2->second.get());
		}
	}
	return "";
}

std::vector<std::string> htmlview_jni_get_viewport_list(const std::string &id)
{
	std::vector<std::string> names;
	std::lock_guard<std::mutex> lock(g_viewport_mutex);
	auto it = g_viewports.find(id);
	if (it != g_viewports.end()) {
		for (const auto &pair : it->second) {
			names.push_back(pair.first);
		}
	}
	return names;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_net_minetest_minetest_HTMLViewManager_nativeGetViewportFrame(
		JNIEnv *env, jclass, jstring id, jstring name)
{
	std::string sid = readJavaString(env, id);
	std::string sname = readJavaString(env, name);
	std::string out = htmlview_jni_get_viewport_frame(sid, sname);
	if (out.empty())
		return nullptr;

	jbyteArray arr = env->NewByteArray(out.size());
	env->SetByteArrayRegion(arr, 0, out.size(), (const jbyte *)out.data());
	return arr;
}

#include "cpp_api/s_htmlview.h"

void htmlview_jni_poll(ScriptApiHTMLView *script)
{
	if (!script)
		return;
	std::deque<HtmlViewMessage> batch;
	std::deque<HtmlViewCapture> cap_batch;
	std::deque<HtmlViewEvent> event_batch;
	{
		std::lock_guard<std::mutex> lock(g_msg_mutex);
		batch.swap(g_messages);
		cap_batch.swap(g_captures);
		event_batch.swap(g_events);
	}
	for (const auto &m : batch) {
		script->on_htmlview_message(m.id, m.message);
	}
	for (const auto &c : cap_batch) {
		script->on_htmlview_capture(c.id, c.png_base64);
	}
	for (const auto &e : event_batch) {
		if (e.type == HtmlViewEvent::READY)
			script->on_htmlview_ready(e.id);
	}
}

#endif // __ANDROID__
