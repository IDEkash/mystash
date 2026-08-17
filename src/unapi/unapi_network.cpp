// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_network.h"
#include "httpfetch.h"
#include "log.h"

namespace unapi {

HttpResponse ExtensionNetwork::fetchHttp(const std::string &extension_id, const std::string &url,
                                        const std::string &method, const ValueMap &headers,
                                        const std::string &post_data, int timeout_ms) {
	HttpResponse resp;
	HTTPFetchRequest req;
	req.url = url;
	req.timeout = timeout_ms;
	req.connect_timeout = timeout_ms;

	if (method == "POST") req.method = HTTP_POST;
	else if (method == "HEAD") req.method = HTTP_HEAD;
	else if (method == "PUT") req.method = HTTP_PUT;
	else if (method == "DELETE") req.method = HTTP_DELETE;
	else req.method = HTTP_GET;

	req.raw_data = post_data;

	for (const auto &pair : headers) {
		req.extra_headers.push_back(pair.first + ": " + pair.second.asString());
	}

	HTTPFetchResult result;
	bool completed = httpfetch_sync_interruptible(req, result);
	resp.completed = completed;
	resp.status_code = static_cast<int>(result.response_code);
	resp.body = result.data;
	if (!result.succeeded) {
		resp.error = result.timeout ? "Request timed out" : "HTTP request failed";
	}
	return resp;
}

} // namespace unapi
