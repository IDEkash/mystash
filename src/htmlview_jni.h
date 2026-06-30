// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include "irrlichttypes_bloated.h"

class ServerScripting;
class Client;

#ifdef __ANDROID__

	void htmlview_jni_run(const std::string &id, const std::string &html);
	void htmlview_jni_run_worker(const std::string &id, const std::string &html);
	void htmlview_jni_run_external(const std::string &id, const std::string &root_dir,
			const std::string &entry);
	void htmlview_jni_run_external_worker(const std::string &id, const std::string &root_dir,
			const std::string &entry);
	void htmlview_jni_stop(const std::string &id);
	void htmlview_jni_shutdown_all();
	void htmlview_jni_reload(const std::string &id);
	void htmlview_jni_focus(const std::string &id);
	std::string htmlview_jni_state(const std::string &id);
	void htmlview_jni_display(const std::string &id, int x, int y, int w, int h,
			bool visible, bool fullscreen, bool safe_area,
			bool drag_embed, float border_radius);
	void htmlview_jni_input(const std::string &id, bool block_game_input);
	void htmlview_jni_send(const std::string &id, const std::string &message);
	void htmlview_jni_navigate(const std::string &id, const std::string &url);
	void htmlview_jni_inject(const std::string &id, const std::string &js);
	void htmlview_jni_pipe(const std::string &fromId, const std::string &toId);
	void htmlview_jni_shared_set(const std::string &key, const char *val);
	std::string htmlview_jni_shared_get(const std::string &key);
	void htmlview_jni_capture(const std::string &id, int width, int height);
	void htmlview_jni_set_viewport(const std::string &id, const std::string &name,
			v3f pos, v3f dir, float fov, int width, int height);
	void htmlview_jni_remove_viewport(const std::string &id, const std::string &name);
	void htmlview_jni_render_viewports(Client *client);
	void htmlview_jni_poll(ServerScripting *script);

#endif
