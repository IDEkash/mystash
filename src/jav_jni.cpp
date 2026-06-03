// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef __ANDROID__

#include "jav_jni.h"
#include "porting_android.h"
#include "log.h"
#include "script/lua_api/l_jav.h"
#include "cpp_api/s_base.h"
#include <mutex>

namespace jav_jni {

static jclass bridge_class;
static jmethodID import_class_mid;
static jmethodID new_instance_mid;
static jmethodID call_static_mid;
static jmethodID call_instance_mid;
static jmethodID get_field_mid;
static jmethodID set_field_mid;
static jmethodID methods_mid;
static jmethodID fields_mid;
static jmethodID poll_async_results_mid;
static jmethodID poll_events_mid;

static jclass async_result_class;
static jfieldID async_callback_id_fid;
static jfieldID async_result_fid;
static jfieldID async_error_fid;

static jclass event_class;
static jfieldID event_name_fid;
static jfieldID event_data_fid;

void initialize(JNIEnv *env)
{
	jclass local_class = env->FindClass("net/minetest/minetest/JavBridge");
	bridge_class = (jclass)env->NewGlobalRef(local_class);

	import_class_mid = env->GetStaticMethodID(bridge_class, "importClass", "(Ljava/lang/String;)Ljava/lang/Object;");
	new_instance_mid = env->GetStaticMethodID(bridge_class, "newInstance", "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;");
	call_static_mid = env->GetStaticMethodID(bridge_class, "callStatic", "(Ljava/lang/Class;Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;");
	call_instance_mid = env->GetStaticMethodID(bridge_class, "callInstance", "(Ljava/lang/Object;Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;");
	get_field_mid = env->GetStaticMethodID(bridge_class, "getField", "(Ljava/lang/Object;Ljava/lang/String;)Ljava/lang/Object;");
	set_field_mid = env->GetStaticMethodID(bridge_class, "setField", "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/Object;)V");
	methods_mid = env->GetStaticMethodID(bridge_class, "methods", "(Ljava/lang/Object;)[Ljava/lang/String;");
	fields_mid = env->GetStaticMethodID(bridge_class, "fields", "(Ljava/lang/Object;)[Ljava/lang/String;");
	poll_async_results_mid = env->GetStaticMethodID(bridge_class, "pollAsyncResults", "()[Ljava/lang/Object;");
	poll_events_mid = env->GetStaticMethodID(bridge_class, "pollEvents", "()[Ljava/lang/Object;");

	async_result_class = (jclass)env->NewGlobalRef(env->FindClass("net/minetest/minetest/JavBridge$AsyncResult"));
	async_callback_id_fid = env->GetFieldID(async_result_class, "callbackId", "J");
	async_result_fid = env->GetFieldID(async_result_class, "result", "Ljava/lang/Object;");
	async_error_fid = env->GetFieldID(async_result_class, "error", "Ljava/lang/String;");

	event_class = (jclass)env->NewGlobalRef(env->FindClass("net/minetest/minetest/JavBridge$JavEvent"));
	event_name_fid = env->GetFieldID(event_class, "name", "Ljava/lang/String;");
	event_data_fid = env->GetFieldID(event_class, "data", "Ljava/lang/Object;");
}

jobject import_class(const std::string &class_name)
{
	JNIEnv *env = porting::getJNIEnv();
	jstring jname = env->NewStringUTF(class_name.c_str());
	jobject res = env->CallStaticObjectMethod(bridge_class, import_class_mid, jname);
	env->DeleteLocalRef(jname);
	return res;
}

jobject new_instance(const std::string &class_name, jobjectArray args)
{
	JNIEnv *env = porting::getJNIEnv();
	jstring jname = env->NewStringUTF(class_name.c_str());
	jobject res = env->CallStaticObjectMethod(bridge_class, new_instance_mid, jname, args);
	env->DeleteLocalRef(jname);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return nullptr;
	}
	return res;
}

jobject call_static(jobject clazz, const std::string &method_name, jobjectArray args)
{
	JNIEnv *env = porting::getJNIEnv();
	jstring jname = env->NewStringUTF(method_name.c_str());
	jobject res = env->CallStaticObjectMethod(bridge_class, call_static_mid, clazz, jname, args);
	env->DeleteLocalRef(jname);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return nullptr;
	}
	return res;
}

jobject call_instance(jobject instance, const std::string &method_name, jobjectArray args)
{
	JNIEnv *env = porting::getJNIEnv();
	jstring jname = env->NewStringUTF(method_name.c_str());
	jobject res = env->CallStaticObjectMethod(bridge_class, call_instance_mid, instance, jname, args);
	env->DeleteLocalRef(jname);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return nullptr;
	}
	return res;
}

jobject get_field(jobject target, const std::string &field_name)
{
	JNIEnv *env = porting::getJNIEnv();
	jstring jname = env->NewStringUTF(field_name.c_str());
	jobject res = env->CallStaticObjectMethod(bridge_class, get_field_mid, target, jname);
	env->DeleteLocalRef(jname);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return nullptr;
	}
	return res;
}

void set_field(jobject target, const std::string &field_name, jobject value)
{
	JNIEnv *env = porting::getJNIEnv();
	jstring jname = env->NewStringUTF(field_name.c_str());
	env->CallStaticVoidMethod(bridge_class, set_field_mid, target, jname, value);
	env->DeleteLocalRef(jname);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
	}
}

std::vector<std::string> methods(jobject target)
{
	std::vector<std::string> res;
	JNIEnv *env = porting::getJNIEnv();
	jobjectArray array = (jobjectArray)env->CallStaticObjectMethod(bridge_class, methods_mid, target);
	if (array) {
		jsize len = env->GetArrayLength(array);
		for (jsize i = 0; i < len; i++) {
			jstring str = (jstring)env->GetObjectArrayElement(array, i);
			const char *c_str = env->GetStringUTFChars(str, nullptr);
			res.push_back(c_str);
			env->ReleaseStringUTFChars(str, c_str);
			env->DeleteLocalRef(str);
		}
		env->DeleteLocalRef(array);
	}
	return res;
}

std::vector<std::string> fields(jobject target)
{
	std::vector<std::string> res;
	JNIEnv *env = porting::getJNIEnv();
	jobjectArray array = (jobjectArray)env->CallStaticObjectMethod(bridge_class, fields_mid, target);
	if (array) {
		jsize len = env->GetArrayLength(array);
		for (jsize i = 0; i < len; i++) {
			jstring str = (jstring)env->GetObjectArrayElement(array, i);
			const char *c_str = env->GetStringUTFChars(str, nullptr);
			res.push_back(c_str);
			env->ReleaseStringUTFChars(str, c_str);
			env->DeleteLocalRef(str);
		}
		env->DeleteLocalRef(array);
	}
	return res;
}

std::vector<AsyncResult> poll_async_results()
{
	std::vector<AsyncResult> res;
	JNIEnv *env = porting::getJNIEnv();
	jobjectArray array = (jobjectArray)env->CallStaticObjectMethod(bridge_class, poll_async_results_mid);
	if (array) {
		jsize len = env->GetArrayLength(array);
		for (jsize i = 0; i < len; i++) {
			jobject obj = env->GetObjectArrayElement(array, i);
			AsyncResult ar;
			ar.callback_id = env->GetLongField(obj, async_callback_id_fid);
			jobject result_obj = env->GetObjectField(obj, async_result_fid);
			ar.result = result_obj ? env->NewGlobalRef(result_obj) : nullptr;
			jstring error_str = (jstring)env->GetObjectField(obj, async_error_fid);
			if (error_str) {
				const char *c_str = env->GetStringUTFChars(error_str, nullptr);
				ar.error = c_str;
				env->ReleaseStringUTFChars(error_str, c_str);
			}
			res.push_back(ar);
			env->DeleteLocalRef(obj);
			if (result_obj) env->DeleteLocalRef(result_obj);
		}
		env->DeleteLocalRef(array);
	}
	return res;
}

std::vector<Event> poll_events()
{
	std::vector<Event> res;
	JNIEnv *env = porting::getJNIEnv();
	jobjectArray array = (jobjectArray)env->CallStaticObjectMethod(bridge_class, poll_events_mid);
	if (array) {
		jsize len = env->GetArrayLength(array);
		for (jsize i = 0; i < len; i++) {
			jobject obj = env->GetObjectArrayElement(array, i);
			Event e;
			jstring name_str = (jstring)env->GetObjectField(obj, event_name_fid);
			const char *c_name = env->GetStringUTFChars(name_str, nullptr);
			e.name = c_name;
			env->ReleaseStringUTFChars(name_str, c_name);
			jobject data_obj = env->GetObjectField(obj, event_data_fid);
			e.data = data_obj ? env->NewGlobalRef(data_obj) : nullptr;
			res.push_back(e);
			env->DeleteLocalRef(obj);
			if (data_obj) env->DeleteLocalRef(data_obj);
		}
		env->DeleteLocalRef(array);
	}
	return res;
}

void poll(ScriptApiBase *script)
{
	if (!script) return;
	ModApiJav::Poll(script->getStack());
}

}

#endif
