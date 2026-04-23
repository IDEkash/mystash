#pragma once

#ifdef __ANDROID__
#include <jni.h>
#include <string>

class Server;
class Client;

class JvmModManager {
public:
	static void init(JNIEnv *env, jobject activity);
	static void loadMods();
	static JNIEnv* getEnv();

	static void setServer(Server *server) { m_server = server; }
	static void setClient(Client *client) { m_client = client; }

private:
	static Server *m_server;
	static Client *m_client;
	static JavaVM *m_vm;
	static jobject m_mod_loader;
	static jmethodID m_load_mods_method;
};

#endif
