#ifdef __ANDROID__
#include "jvm_mod_manager.h"
#include "log.h"
#include "porting.h"
#include "settings.h"
#include "filesys.h"
#include "server.h"
#include "client/client.h"
#include "serverenvironment.h"
#include "server/luaentity_sao.h"
#include <SDL.h>

jobject JvmModManager::m_mod_loader = nullptr;
jmethodID JvmModManager::m_load_mods_method = nullptr;
JavaVM *JvmModManager::m_vm = nullptr;
Server *JvmModManager::m_server = nullptr;
Client *JvmModManager::m_client = nullptr;

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
	infostream << "JvmModManager: Initializing JNI bridge" << std::endl;

	jclass modLoaderClass = env->FindClass("net/minetest/minetest/jvm/ModLoader");
	if (modLoaderClass == nullptr) {
		errorstream << "JvmModManager: Could not find ModLoader class" << std::endl;
		return;
	}

	jmethodID constructor = env->GetMethodID(modLoaderClass, "<init>", "(Landroid/content/Context;)V");
	if (constructor == nullptr) {
		errorstream << "JvmModManager: Could not find ModLoader constructor" << std::endl;
		return;
	}

	jobject localModLoader = env->NewObject(modLoaderClass, constructor, activity);
	m_mod_loader = env->NewGlobalRef(localModLoader);

	m_load_mods_method = env->GetMethodID(modLoaderClass, "loadMods", "()V");
	if (m_load_mods_method == nullptr) {
		errorstream << "JvmModManager: Could not find loadMods method" << std::endl;
	}
}

void JvmModManager::loadMods()
{
	if (m_mod_loader != nullptr && m_load_mods_method != nullptr) {
		infostream << "JvmModManager: Triggering ModLoader.loadMods()" << std::endl;
		getEnv()->CallVoidMethod(m_mod_loader, m_load_mods_method);
	} else {
		errorstream << "JvmModManager: Cannot load mods, loader not initialized" << std::endl;
	}
}

// JNI Implementation for EngineAPIImpl

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_spawnEntity(JNIEnv* env, jobject /* this */, jstring id, jfloat x, jfloat y, jfloat z) {
    const char *nativeId = env->GetStringUTFChars(id, 0);
    actionstream << "JNI-API: spawnEntity(" << nativeId << ", " << x << ", " << y << ", " << z << ")" << std::endl;

	if (JvmModManager::m_server) {
		v3f pos(x, y, z);
		auto sao = std::make_unique<LuaEntitySAO>(&JvmModManager::m_server->getEnv(), pos, nativeId, "");
		JvmModManager::m_server->getEnv().addActiveObject(std::move(sao));
	} else {
		errorstream << "JNI-API: Cannot spawn entity, server not running" << std::endl;
	}

    env->ReleaseStringUTFChars(id, nativeId);
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_registerModelFormat(JNIEnv* env, jobject /* this */, jstring extension, jobject parser) {
    const char *nativeExt = env->GetStringUTFChars(extension, 0);
    actionstream << "JNI-API: registerModelFormat(." << nativeExt << ")" << std::endl;
    env->ReleaseStringUTFChars(extension, nativeExt);
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_registerMesh(JNIEnv* env, jobject /* this */, jstring name, jbyteArray data) {
    const char *nativeName = env->GetStringUTFChars(name, 0);
    actionstream << "JNI-API: registerMesh(" << nativeName << ")" << std::endl;

	if (JvmModManager::m_client) {
		jbyte* buffer = env->GetByteArrayElements(data, NULL);
		jsize length = env->GetArrayLength(data);

		JvmModManager::m_client->m_mesh_data[nativeName] = std::string((char*)buffer, length);

		env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);
	} else {
		errorstream << "JNI-API: Cannot register mesh, client not running" << std::endl;
	}

    env->ReleaseStringUTFChars(name, nativeName);
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_setFOV(JNIEnv* env, jobject /* this */, jint fov) {
    actionstream << "JNI-API: setFOV(" << fov << ")" << std::endl;
    g_settings->set("fov", std::to_string(fov));
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_readFile(JNIEnv* env, jobject /* this */, jstring path) {
    const char *nativePath = env->GetStringUTFChars(path, 0);
    verbosestream << "JNI-API: readFile(" << nativePath << ")" << std::endl;

    std::string content;
    bool success = fs::ReadFile(nativePath, content);

    env->ReleaseStringUTFChars(path, nativePath);

	if (!success)
		return nullptr;

	jbyteArray array = env->NewByteArray(content.size());
	env->SetByteArrayRegion(array, 0, content.size(), (const jbyte*)content.data());
    return array;
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_writeFile(JNIEnv* env, jobject /* this */, jstring path, jbyteArray data) {
    const char *nativePath = env->GetStringUTFChars(path, 0);
    actionstream << "JNI-API: writeFile(" << nativePath << ")" << std::endl;

    jbyte* buffer = env->GetByteArrayElements(data, NULL);
    jsize length = env->GetArrayLength(data);

    fs::safeWriteToFile(nativePath, std::string((char*)buffer, length));

    env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);
    env->ReleaseStringUTFChars(path, nativePath);
}

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_jvm_EngineAPIImpl_log(JNIEnv* env, jobject /* this */, jstring message) {
    const char *nativeMsg = env->GetStringUTFChars(message, 0);
    actionstream << "JVM-MOD: " << nativeMsg << std::endl;
    env->ReleaseStringUTFChars(message, nativeMsg);
}

#endif
