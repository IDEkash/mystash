// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include "irrlichttypes.h"
#include <IImage.h>
#include <ITexture.h>

class StreamTextureManager {
public:
	static StreamTextureManager &get()
	{
		static StreamTextureManager instance;
		return instance;
	}

	void update();
	void registerStream(const std::string &id, video::ITexture *texture);
	void unregisterStream(const std::string &id);
	void cleanup();

private:
	StreamTextureManager() = default;
	~StreamTextureManager() = default;

	struct StreamInfo {
		video::ITexture *texture;
		u32 width;
		u32 height;
	};

	std::unordered_map<std::string, StreamInfo> m_streams;
	std::mutex m_mutex;
};
