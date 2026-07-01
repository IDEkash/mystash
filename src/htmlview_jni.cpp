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
#include "irr_ptr.h"

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
}

void htmlview_jni_shutdown_all()
{
	callVoidMethod0("htmlview_shutdown_all");
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

void htmlview_jni_render_viewports(Client *client, float dtime)
{
	std::lock_guard<std::mutex> lock(g_viewport_mutex);
	auto driver = RenderingEngine::get_video_driver();
	auto smgr = RenderingEngine::get_raw_device()->getSceneManager();

	for (auto &pair : g_viewports) {
		auto &vps = pair.second;
		for (auto it = vps.begin(); it != vps.end(); ) {
			auto &vp = it->second;
			if (vp->pending_removal) {
				if (vp->texture)
					driver->removeTexture(vp->texture);
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
					// Save current camera state
					auto old_cam_node = smgr->getActiveCamera();
					v3f old_pos = old_cam_node->getPosition();
					v3f old_target = old_cam_node->getTarget();
					v3f old_up = old_cam_node->getUpVector();
					float old_fov = old_cam_node->getFOV();
					float old_aspect = old_cam_node->getAspectRatio();

					// Setup viewport camera
					old_cam_node->setPosition(vp->pos);
					old_cam_node->setTarget(vp->pos + vp->dir * 100.0f);

					v3f up = vp->up;
					if (vp->tilt != 0.0f) {
						core::quaternion q;
						q.fromAngleAxis(vp->tilt * core::DEGTORAD, vp->dir);
						up = q * up;
					}
					old_cam_node->setUpVector(up);

					old_cam_node->setFOV(vp->fov * core::DEGTORAD);
					old_cam_node->setAspectRatio((float)vp->width / vp->height);
					old_cam_node->updateMatrices();

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

					// Restore camera state
					old_cam_node->setPosition(old_pos);
					old_cam_node->setTarget(old_target);
					old_cam_node->setUpVector(old_up);
					old_cam_node->setFOV(old_fov);
					old_cam_node->setAspectRatio(old_aspect);
					old_cam_node->updateMatrices();
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

static std::string encode_viewport_frame(Viewport *vp)
{
	std::lock_guard<std::mutex> img_lock(vp->image_mutex);
	if (!vp->image)
		return "";

	std::string out;
	if (vp->format == "png") {
		auto size = vp->image->getDimension();
		u32 pixel_count = size.Width * size.Height;
		std::vector<u8> rgba_data(pixel_count * 4);
		u8 *src = (u8 *)vp->image->getData();
		for (u32 i = 0; i < pixel_count; ++i) {
			rgba_data[i * 4 + 0] = src[i * 4 + 2]; // R
			rgba_data[i * 4 + 1] = src[i * 4 + 1]; // G
			rgba_data[i * 4 + 2] = src[i * 4 + 0]; // B
			rgba_data[i * 4 + 3] = src[i * 4 + 3]; // A
		}
		out = encodePNG(rgba_data.data(), size.Width, size.Height, 6);
	} else {
		// Use Irrlicht's JPEG writer
		auto driver = RenderingEngine::get_video_driver();
		auto fs = RenderingEngine::get_raw_device()->getFileSystem();

		// Max size for the buffer: width * height * 3 (uncompressed) should be enough for JPEG
		u32 max_size = vp->width * vp->height * 3 + 1024;
		std::vector<u8> buffer(max_size);

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
			auto size = vp->image->getDimension();
			u32 pixel_count = size.Width * size.Height;
			std::vector<u8> rgba_data(pixel_count * 4);
			u8 *src = (u8 *)vp->image->getData();
			for (u32 i = 0; i < pixel_count; ++i) {
				rgba_data[i * 4 + 0] = src[i * 4 + 2]; // R
				rgba_data[i * 4 + 1] = src[i * 4 + 1]; // G
				rgba_data[i * 4 + 2] = src[i * 4 + 0]; // B
				rgba_data[i * 4 + 3] = src[i * 4 + 3]; // A
			}
			out = encodePNG(rgba_data.data(), size.Width, size.Height, 6);
		}
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
