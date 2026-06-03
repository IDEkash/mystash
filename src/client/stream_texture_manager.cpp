// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "stream_texture_manager.h"
#include "htmlview_jni.h"
#include "log.h"
#include "renderingengine.h"
#include <IVideoDriver.h>

void StreamTextureManager::update()
{
#ifdef __ANDROID__
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto &it : m_streams) {
		const std::string &id = it.first;
		StreamInfo &info = it.second;

		if (!htmlview_jni_is_stream_dirty(id))
			continue;

		u32 expected_size = info.width * info.height * 4;
		void *data = info.texture->lock(video::ETLM_WRITE_ONLY);
		if (data) {
			if (!htmlview_jni_get_stream_pixels(id, data, expected_size)) {
				info.texture->unlock();
				continue;
			}
			info.texture->unlock();
			info.texture->regenerateMipMapLevels();
		}
	}
#endif
}

void StreamTextureManager::registerStream(const std::string &id, video::ITexture *texture)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	StreamInfo info;
	info.texture = texture;
	core::dimension2du dim = texture->getSize();
	info.width = dim.Width;
	info.height = dim.Height;
	m_streams[id] = info;

#ifdef __ANDROID__
	htmlview_jni_set_stream_target_size(id, info.width, info.height);
#endif
}

void StreamTextureManager::unregisterStream(const std::string &id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_streams.erase(id);
}

void StreamTextureManager::cleanup()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_streams.clear();
}
