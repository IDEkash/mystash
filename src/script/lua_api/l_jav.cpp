// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_jav.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "log.h"

#ifdef __ANDROID__
#include "jav_jni.h"
#include "porting_android.h"
#include "cpp_api/s_base.h"

struct JavObjectProxy {
	jobject obj;
};

static int jav_object_gc(lua_State *L)
{
	JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
	if (proxy->obj) {
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteGlobalRef(proxy->obj);
		proxy->obj = nullptr;
	}
	return 0;
}

static jobject lua_to_java(lua_State *L, int idx);
static void java_to_lua(lua_State *L, jobject obj);

static jobjectArray lua_to_java_array(lua_State *L, int start, int end)
{
	JNIEnv *env = porting::getJNIEnv();
	int count = end - start + 1;
	if (count < 0) count = 0;
	jobjectArray array = env->NewObjectArray(count, env->FindClass("java/lang/Object"), nullptr);
	for (int i = 0; i < count; i++) {
		jobject obj = lua_to_java(L, start + i);
		env->SetObjectArrayElement(array, i, obj);
		if (obj) env->DeleteLocalRef(obj);
	}
	return array;
}

static jobject lua_to_java(lua_State *L, int idx)
{
	JNIEnv *env = porting::getJNIEnv();
	int type = lua_type(L, idx);
	if (type == LUA_TNUMBER) {
		double v = lua_tonumber(L, idx);
		jclass clazz = env->FindClass("java/lang/Double");
		jmethodID mid = env->GetMethodID(clazz, "<init>", "(D)V");
		return env->NewObject(clazz, mid, (jdouble)v);
	} else if (type == LUA_TSTRING) {
		return env->NewStringUTF(lua_tostring(L, idx));
	} else if (type == LUA_TBOOLEAN) {
		jclass clazz = env->FindClass("java/lang/Boolean");
		jmethodID mid = env->GetStaticMethodID(clazz, "valueOf", "(Z)Ljava/lang/Boolean;");
		return env->CallStaticObjectMethod(clazz, mid, (jboolean)lua_toboolean(L, idx));
	} else if (type == LUA_TUSERDATA) {
		JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, idx, "jav_object");
		return env->NewLocalRef(proxy->obj);
	} else if (type == LUA_TTABLE) {
		// Detect if it is an array-like table
		lua_rawgeti(L, idx < 0 ? idx : idx, 1);
		bool is_array = !lua_isnil(L, -1);
		lua_pop(L, 1);

		if (is_array) {
			int count = lua_objlen(L, idx);
			jobjectArray array = env->NewObjectArray(count, env->FindClass("java/lang/Object"), nullptr);
			for (int i = 0; i < count; i++) {
				lua_rawgeti(L, idx < 0 ? idx : idx, i + 1);
				jobject obj = lua_to_java(L, -1);
				env->SetObjectArrayElement(array, i, obj);
				if (obj) env->DeleteLocalRef(obj);
				lua_pop(L, 1);
			}
			return array;
		}

		// Basic conversion to HashMap
		jclass map_class = env->FindClass("java/util/HashMap");
		jmethodID map_init = env->GetMethodID(map_class, "<init>", "()V");
		jmethodID map_put = env->GetMethodID(map_class, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
		jobject map = env->NewObject(map_class, map_init);
		lua_pushnil(L);
		while (lua_next(L, idx < 0 ? idx - 1 : idx) != 0) {
			jobject key = lua_to_java(L, -2);
			jobject val = lua_to_java(L, -1);
			env->CallObjectMethod(map, map_put, key, val);
			if (key) env->DeleteLocalRef(key);
			if (val) env->DeleteLocalRef(val);
			lua_pop(L, 1);
		}
		return map;
	}
	return nullptr;
}

static void java_to_lua(lua_State *L, jobject obj)
{
	if (!obj) {
		lua_pushnil(L);
		return;
	}
	JNIEnv *env = porting::getJNIEnv();
	jclass obj_class = env->GetObjectClass(obj);

	// Check for primitives wrappers
	jclass string_class = env->FindClass("java/lang/String");
	if (env->IsInstanceOf(obj, string_class)) {
		const char *c_str = env->GetStringUTFChars((jstring)obj, nullptr);
		lua_pushstring(L, c_str);
		env->ReleaseStringUTFChars((jstring)obj, c_str);
		return;
	}
	jclass double_class = env->FindClass("java/lang/Double");
	if (env->IsInstanceOf(obj, double_class)) {
		jmethodID mid = env->GetMethodID(double_class, "doubleValue", "()D");
		lua_pushnumber(L, env->CallDoubleMethod(obj, mid));
		return;
	}
	jclass integer_class = env->FindClass("java/lang/Integer");
	if (env->IsInstanceOf(obj, integer_class)) {
		jmethodID mid = env->GetMethodID(integer_class, "intValue", "()I");
		lua_pushinteger(L, env->CallIntMethod(obj, mid));
		return;
	}
	jclass boolean_class = env->FindClass("java/lang/Boolean");
	if (env->IsInstanceOf(obj, boolean_class)) {
		jmethodID mid = env->GetMethodID(boolean_class, "booleanValue", "()Z");
		lua_pushboolean(L, env->CallBooleanMethod(obj, mid));
		return;
	}

	// Array handling
	jclass obj_array_class = env->FindClass("[Ljava/lang/Object;");
	if (env->IsInstanceOf(obj, obj_array_class)) {
		jobjectArray array = (jobjectArray)obj;
		jsize len = env->GetArrayLength(array);
		lua_newtable(L);
		for (jsize i = 0; i < len; i++) {
			jobject element = env->GetObjectArrayElement(array, i);
			java_to_lua(L, element);
			lua_rawseti(L, -2, i + 1);
			if (element) env->DeleteLocalRef(element);
		}
		return;
	}

	// List handling
	jclass list_class = env->FindClass("java/util/List");
	if (env->IsInstanceOf(obj, list_class)) {
		jmethodID list_size = env->GetMethodID(list_class, "size", "()I");
		jmethodID list_get = env->GetMethodID(list_class, "get", "(I)Ljava/lang/Object;");
		int size = env->CallIntMethod(obj, list_size);
		lua_newtable(L);
		for (int i = 0; i < size; i++) {
			jobject element = env->CallObjectMethod(obj, list_get, i);
			java_to_lua(L, element);
			lua_rawseti(L, -2, i + 1);
			if (element) env->DeleteLocalRef(element);
		}
		return;
	}

	// Map handling
	jclass map_interface = env->FindClass("java/util/Map");
	if (env->IsInstanceOf(obj, map_interface)) {
		jmethodID map_keyset = env->GetMethodID(map_interface, "keySet", "()Ljava/util/Set;");
		jmethodID map_get = env->GetMethodID(map_interface, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
		jclass set_class = env->FindClass("java/util/Set");
		jmethodID set_iterator = env->GetMethodID(set_class, "iterator", "()Ljava/util/Iterator;");
		jclass iter_class = env->FindClass("java/util/Iterator");
		jmethodID iter_hasnext = env->GetMethodID(iter_class, "hasNext", "()Z");
		jmethodID iter_next = env->GetMethodID(iter_class, "next", "()Ljava/lang/Object;");

		jobject keyset = env->CallObjectMethod(obj, map_keyset);
		jobject iter = env->CallObjectMethod(keyset, set_iterator);
		lua_newtable(L);
		while (env->CallBooleanMethod(iter, iter_hasnext)) {
			jobject key = env->CallObjectMethod(iter, iter_next);
			jobject val = env->CallObjectMethod(obj, map_get, key);
			java_to_lua(L, key);
			java_to_lua(L, val);
			lua_settable(L, -3);
			if (key) env->DeleteLocalRef(key);
			if (val) env->DeleteLocalRef(val);
		}
		env->DeleteLocalRef(iter);
		env->DeleteLocalRef(keyset);
		return;
	}

	// Default: wrap as proxy
	JavObjectProxy *proxy = (JavObjectProxy *)lua_newuserdata(L, sizeof(JavObjectProxy));
	proxy->obj = env->NewGlobalRef(obj);
	luaL_getmetatable(L, "jav_object");
	lua_setmetatable(L, -2);
}

static int jav_object_index(lua_State *L)
{
	JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
	const char *name = luaL_checkstring(L, 2);

	// Try to see if it's a field
	jobject field_val = jav_jni::get_field(proxy->obj, name);
	if (field_val) {
		java_to_lua(L, field_val);
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteLocalRef(field_val);
		return 1;
	}

	// Otherwise return a closure for method call
	lua_pushstring(L, name);
	lua_pushcclosure(L, [](lua_State *L) -> int {
		JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
		const char *method_name = lua_tostring(L, lua_upvalueindex(1));
		jobjectArray args = lua_to_java_array(L, 2, lua_gettop(L));
		jobject res = jav_jni::call_instance(proxy->obj, method_name, args);
		java_to_lua(L, res);
		JNIEnv *env = porting::getJNIEnv();
		if (res) env->DeleteLocalRef(res);
		env->DeleteLocalRef(args);
		return 1;
	}, 1);
	return 1;
}

static int jav_object_newindex(lua_State *L)
{
	JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
	const char *name = luaL_checkstring(L, 2);
	jobject val = lua_to_java(L, 3);
	jav_jni::set_field(proxy->obj, name, val);
	if (val) {
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteLocalRef(val);
	}
	return 0;
}

int ModApiJav::l_import(lua_State *L)
{
	const char *class_name = luaL_checkstring(L, 1);
	jobject clazz = jav_jni::import_class(class_name);
	if (!clazz) {
		lua_pushnil(L);
		return 1;
	}
	java_to_lua(L, clazz);
	JNIEnv *env = porting::getJNIEnv();
	env->DeleteLocalRef(clazz);
	return 1;
}

int ModApiJav::l_new(lua_State *L)
{
	const char *class_name = luaL_checkstring(L, 1);
	jobjectArray args = lua_to_java_array(L, 2, lua_gettop(L));
	jobject res = jav_jni::new_instance(class_name, args);
	java_to_lua(L, res);
	JNIEnv *env = porting::getJNIEnv();
	if (res) env->DeleteLocalRef(res);
	env->DeleteLocalRef(args);
	return 1;
}

int ModApiJav::l_call(lua_State *L)
{
	const char *class_name = luaL_checkstring(L, 1);
	const char *method_name = luaL_checkstring(L, 2);
	jobject clazz = jav_jni::import_class(class_name);
	if (!clazz) return 0;
	jobjectArray args = lua_to_java_array(L, 3, lua_gettop(L));
	jobject res = jav_jni::call_static(clazz, method_name, args);
	java_to_lua(L, res);
	JNIEnv *env = porting::getJNIEnv();
	env->DeleteLocalRef(clazz);
	if (res) env->DeleteLocalRef(res);
	env->DeleteLocalRef(args);
	return 1;
}

int ModApiJav::l_get(lua_State *L)
{
	if (lua_isstring(L, 1)) {
		std::string full_name = lua_tostring(L, 1);
		size_t last_dot = full_name.find_last_of('.');
		if (last_dot == std::string::npos) return 0;
		std::string class_name = full_name.substr(0, last_dot);
		std::string field_name = full_name.substr(last_dot + 1);
		jobject clazz = jav_jni::import_class(class_name);
		if (!clazz) return 0;
		jobject res = jav_jni::get_field(clazz, field_name);
		java_to_lua(L, res);
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteLocalRef(clazz);
		if (res) env->DeleteLocalRef(res);
		return 1;
	}
	JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
	const char *name = luaL_checkstring(L, 2);
	jobject res = jav_jni::get_field(proxy->obj, name);
	java_to_lua(L, res);
	JNIEnv *env = porting::getJNIEnv();
	if (res) env->DeleteLocalRef(res);
	return 1;
}

int ModApiJav::l_set(lua_State *L)
{
	JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
	const char *name = luaL_checkstring(L, 2);
	jobject val = lua_to_java(L, 3);
	jav_jni::set_field(proxy->obj, name, val);
	if (val) {
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteLocalRef(val);
	}
	return 0;
}

int ModApiJav::l_methods(lua_State *L)
{
	jobject target;
	if (lua_isstring(L, 1)) {
		target = jav_jni::import_class(lua_tostring(L, 1));
	} else {
		JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
		target = proxy->obj;
	}
	if (!target) return 0;
	std::vector<std::string> res = jav_jni::methods(target);
	lua_newtable(L);
	for (size_t i = 0; i < res.size(); i++) {
		lua_pushstring(L, res[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}
	if (lua_isstring(L, 1)) {
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteLocalRef(target);
	}
	return 1;
}

int ModApiJav::l_classes(lua_State *L)
{
	// Return a few common classes
	const char *classes[] = {"android.widget.Toast", "android.content.Intent", "android.os.Build", "android.os.BatteryManager"};
	lua_newtable(L);
	for (int i = 0; i < 4; i++) {
		lua_pushstring(L, classes[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

int ModApiJav::l_help(lua_State *L)
{
	const char *class_name = luaL_checkstring(L, 1);
	lua_pushstring(L, "Reflection-based help is available via jav.methods and jav.fields");
	return 1;
}

int ModApiJav::l_fields(lua_State *L)
{
	jobject target;
	if (lua_isstring(L, 1)) {
		target = jav_jni::import_class(lua_tostring(L, 1));
	} else {
		JavObjectProxy *proxy = (JavObjectProxy *)luaL_checkudata(L, 1, "jav_object");
		target = proxy->obj;
	}
	if (!target) return 0;
	std::vector<std::string> res = jav_jni::fields(target);
	lua_newtable(L);
	for (size_t i = 0; i < res.size(); i++) {
		lua_pushstring(L, res[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}
	if (lua_isstring(L, 1)) {
		JNIEnv *env = porting::getJNIEnv();
		env->DeleteLocalRef(target);
	}
	return 1;
}

static long next_callback_id = 1;
static std::map<long, int> async_callbacks;
static std::map<std::string, std::vector<int>> event_handlers;

int ModApiJav::l_async(lua_State *L)
{
	// jav.async(func, callback)
	luaL_checktype(L, 1, LUA_TFUNCTION);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	long id = next_callback_id++;

	lua_pushvalue(L, 2);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	async_callbacks[id] = ref;

	// For a real implementation, we would need to run the Lua function in another thread.
	// However, Luanti's Lua state is not thread-safe.
	// As a compromise, we'll execute the function in the next step and call the callback,
	// or provide a way to do truly async Java calls.

	// Simulation for now:
	lua_pushvalue(L, 1);
	if (lua_pcall(L, 0, 1, 0) == 0) {
		jobject res = lua_to_java(L, -1);
		jav_jni::AsyncResult ar;
		ar.callback_id = id;
		ar.result = res ? porting::getJNIEnv()->NewGlobalRef(res) : nullptr;
		if (res) porting::getJNIEnv()->DeleteLocalRef(res);

		// In a real async scenario, this would be posted from another thread.
		// Since we're in the same thread, we can just manually trigger the callback logic in Poll.
		// But let's just use the result immediately for this demo-level implementation.
	}

	return 0;
}

int ModApiJav::l_on(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	lua_pushvalue(L, 2);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	event_handlers[name].push_back(ref);
	return 0;
}

#endif

void ModApiJav::Initialize(lua_State *L, int top)
{
#ifdef __ANDROID__
	luaL_newmetatable(L, "jav_object");
	lua_pushcfunction(L, jav_object_gc);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, jav_object_index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, jav_object_newindex);
	lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);

	lua_newtable(L);
	int tbl = lua_gettop(L);
	registerFunction(L, "import", l_import, tbl);
	registerFunction(L, "new", l_new, tbl);
	registerFunction(L, "call", l_call, tbl);
	registerFunction(L, "get", l_get, tbl);
	registerFunction(L, "set", l_set, tbl);
	registerFunction(L, "methods", l_methods, tbl);
	registerFunction(L, "fields", l_fields, tbl);
	registerFunction(L, "classes", l_classes, tbl);
	registerFunction(L, "help", l_help, tbl);
	registerFunction(L, "async", l_async, tbl);
	registerFunction(L, "on", l_on, tbl);

	lua_pushvalue(L, tbl);
	lua_setglobal(L, "jav");
	lua_setfield(L, top, "jav");
#endif
}

void ModApiJav::Poll(lua_State *L)
{
#ifdef __ANDROID__
	// Poll async results
	auto results = jav_jni::poll_async_results();
	for (const auto &res : results) {
		auto it = async_callbacks.find(res.callback_id);
		if (it != async_callbacks.end()) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
			if (res.error.empty()) {
				java_to_lua(L, res.result);
				lua_pushnil(L);
			} else {
				lua_pushnil(L);
				lua_pushstring(L, res.error.c_str());
			}
			if (lua_pcall(L, 2, 0, 0) != 0) {
				errorstream << "Error in jav.async callback: " << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
			luaL_unref(L, LUA_REGISTRYINDEX, it->second);
			async_callbacks.erase(it);
		}
		if (res.result) {
			JNIEnv *env = porting::getJNIEnv();
			env->DeleteGlobalRef(res.result);
		}
	}

	// Poll events
	auto events = jav_jni::poll_events();
	for (const auto &e : events) {
		auto it = event_handlers.find(e.name);
		if (it != event_handlers.end()) {
			for (int ref : it->second) {
				lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
				java_to_lua(L, e.data);
				if (lua_pcall(L, 1, 0, 0) != 0) {
					errorstream << "Error in jav.on handler for " << e.name << ": " << lua_tostring(L, -1) << std::endl;
					lua_pop(L, 1);
				}
			}
		}
		if (e.data) {
			JNIEnv *env = porting::getJNIEnv();
			env->DeleteGlobalRef(e.data);
		}
	}
#endif
}
