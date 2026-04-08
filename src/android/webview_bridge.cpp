// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "android/webview_bridge.h"
#include "engine/registry.h"
#include "htmlview_jni.h"
#include "log.h"

WebViewBridge* WebViewBridge::m_instance = nullptr;

void WebViewBridge::Initialize() {
#ifdef __ANDROID__
	m_instance = new WebViewBridge();
	EngineRegistry::expose_raw("webview", m_instance);

	EngineRegistry::expose_method("webview", "open", &WebViewBridge::open);
	EngineRegistry::expose_method("webview", "send", &WebViewBridge::send);
	EngineRegistry::expose_method("webview", "close", &WebViewBridge::close);
#endif
}

void WebViewBridge::open(const std::string &id, const std::string &url) {
#ifdef __ANDROID__
	htmlview_jni_navigate(id, url);
#else
	infostream << "WebViewBridge::open: Only supported on Android. URL: " << url << std::endl;
#endif
}

void WebViewBridge::send(const std::string &id, const std::string &message) {
#ifdef __ANDROID__
	htmlview_jni_send(id, message);
#endif
}

void WebViewBridge::close(const std::string &id) {
#ifdef __ANDROID__
	htmlview_jni_stop(id);
#endif
}
