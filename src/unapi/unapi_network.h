// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "unapi_types.h"
#include <string>
#include <functional>

namespace unapi {

struct HttpResponse {
	int status_code {0};
	std::string body;
	ValueMap headers;
	bool completed {false};
	std::string error;
};

class ExtensionNetwork {
public:
	static HttpResponse fetchHttp(const std::string &extension_id, const std::string &url,
	                             const std::string &method = "GET", const ValueMap &headers = {},
	                             const std::string &post_data = "", int timeout_ms = 5000);
};

} // namespace unapi
