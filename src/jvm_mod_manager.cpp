#ifdef __ANDROID__
#include "jvm_mod_manager.h"
#include "log.h"
#include "porting.h"
#include "settings.h"
#include "filesys.h"
#include <SDL.h>

jobject JvmModManager::m_mod_loader = nullptr;
jmethodID JvmModManager::m_load_mods_method = nullptr;
JavaVM *JvmModManager::m_vm = nullptr;

JNIEnv* JvmModManager::getEnv()
{
	JNIEnv *env;
	if (m_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
		m_vm->AttachCurrentThread(&env, NULL);
	}
	return env;
}

void JvmModManager::init(JNIEnv *env, jobject activity)
{
	env->GetJavaVM(&m_vm);
	infostream << "Initializing JvmModManager" << std::endl;

	jclass modLoaderClass = env->FindClass("net/minetest/minetest/jvm/ModLoader");
	if (modLoaderClass == nullptr) {
		errorstream << "Could not find ModLoader class" << std::endl;
		return;
	}

	jmethodID constructor = env->GetMethodID(modLoaderClass, "<init>", "(Landroid/content/Context;)V");
	if (constructor == nullptr) {
		errorstream << "Could not find ModLoader constructor" << std::endl;
		return;
	}

	jobject localModLoader = env->NewObject(modLoaderClass, constructor, activity);
	m_mod_loader = env->NewGlobalRef(localModLoader);

	m_load_mods_method = env->GetMethodID(modLoaderClass, "loadMods", "()V");
	if (m_load_mods_method == nullptr) {
		errorstream << "Could not find loadMods method" << std::endl;
	}
}

void JvmModManager::loadMods()
{
	if (m_mod_loader != nullptr && m_load_mods_method != nullptr) {
		infostream << "JvmModManager: Loading mods" << std::endl;
		getEnv()->CallVoidMethod(m_mod_loader, m_load_mods_method);
	}
}

// JNI Implementation for EngineAPIImpl

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_spawnEntity(JNIEnv* env, jobject /* this */, jstring id, jfloat x, jfloat y, jfloat z) {
    const char *nativeId = env->GetStringUTFChars(id, 0);
    infostream << "JNI: spawnEntity " << nativeId << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
    // TODO: Implement actual engine hook
    env->ReleaseStringUTFChars(id, nativeId);
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_registerModelFormat(JNIEnv* env, jobject /* this */, jstring extension, jobject parser) {
    const char *nativeExt = env->GetStringUTFChars(extension, 0);
    infostream << "JNI: registerModelFormat for ." << nativeExt << std::endl;
    // TODO: Implement actual engine hook
    env->ReleaseStringUTFChars(extension, nativeExt);
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_setFOV(JNIEnv* env, jobject /* this */, jint fov) {
    infostream << "JNI: setFOV to " << fov << std::endl;
    g_settings->set("fov", std::to_string(fov));
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_readFile(JNIEnv* env, jobject /* this */, jstring path) {
    const char *nativePath = env->GetStringUTFChars(path, 0);
    infostream << "JNI: readFile " << nativePath << std::endl;

    std::string content;
    bool success = fs::ReadFile(nativePath, content);

    env->ReleaseStringUTFChars(path, nativePath);
    return env->NewStringUTF(success ? content.c_str() : "");
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_writeFile(JNIEnv* env, jobject /* this */, jstring path, jbyteArray data) {
    const char *nativePath = env->GetStringUTFChars(path, 0);
    infostream << "JNI: writeFile " << nativePath << std::endl;

    jbyte* buffer = env->GetByteArrayElements(data, NULL);
    jsize length = env->GetArrayLength(data);

    fs::safeWriteToFile(nativePath, std::string((char*)buffer, length));

    env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);
    env->ReleaseStringUTFChars(path, nativePath);
}

#endif
