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
	std::string str(c_str ? c_str : "");
	if (c_str)
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

#include "scripting_server.h"

void htmlview_jni_poll(ServerScripting *script)
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
