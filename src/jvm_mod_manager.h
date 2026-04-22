#pragma once

#ifdef __ANDROID__
#include <jni.h>
#include <string>

class JvmModManager {
public:
	static void init(JNIEnv *env, jobject activity);
	static void loadMods();
	static JNIEnv* getEnv();

private:
	static JavaVM *m_vm;
	static jobject m_mod_loader;
	static jmethodID m_load_mods_method;
};

#endif
