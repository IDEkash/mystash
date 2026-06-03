// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifdef __ANDROID__

#include <jni.h>
#include <string>
#include <vector>

class ScriptApiBase;

namespace jav_jni {

struct AsyncResult {
	long callback_id;
	jobject result;
	std::string error;
};

struct Event {
	std::string name;
	jobject data;
};

void initialize(JNIEnv *env);

jobject import_class(const std::string &class_name);
jobject new_instance(const std::string &class_name, jobjectArray args);
jobject call_static(jobject clazz, const std::string &method_name, jobjectArray args);
jobject call_instance(jobject instance, const std::string &method_name, jobjectArray args);
jobject get_field(jobject target, const std::string &field_name);
void set_field(jobject target, const std::string &field_name, jobject value);

std::vector<std::string> methods(jobject target);
std::vector<std::string> fields(jobject target);

std::vector<AsyncResult> poll_async_results();
std::vector<Event> poll_events();

void poll(ScriptApiBase *script);

}

#endif
