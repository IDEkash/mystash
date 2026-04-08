// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include "engine/meta_macros.h"

class WebViewBridge {
	REFLECT_CLASS()
public:
	static void Initialize();

	// Methods to be called from Lua via Meta-API
	void open(const std::string &id, const std::string &url);
	void send(const std::string &id, const std::string &message);
	void close(const std::string &id);

private:
	static WebViewBridge *m_instance;
};
