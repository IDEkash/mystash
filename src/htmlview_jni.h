// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include "irrlichttypes_bloated.h"

class ServerScripting;
class Client;
class ScriptApiHTMLView;

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
			v3f pos, v3f dir, v3f up, float fov, float tilt, int width, int height,
			u32 refresh_interval_ms, const std::string &format, int quality,
			bool smooth_pos, bool smooth_rot, float pos_smooth, float rot_smooth,
			const std::string &update_mode);
	void htmlview_jni_remove_viewport(const std::string &id, const std::string &name);
	bool htmlview_jni_get_viewport(const std::string &id, const std::string &name,
			v3f &pos, v3f &dir, v3f &up, float &fov, float &tilt, int &width, int &height,
			u32 &refresh_interval_ms, std::string &format, int &quality,
			bool &smooth_pos, bool &smooth_rot, float &pos_smooth, float &rot_smooth,
			std::string &update_mode);
	std::string htmlview_jni_get_viewport_frame(const std::string &id, const std::string &name);
	std::vector<std::string> htmlview_jni_get_viewport_list(const std::string &id);
	void htmlview_jni_set_anchor(const std::string &id, const std::string &type,
			v3f pos, u16 object_id, v3f offset, v2f size, v3f rotation);
	void htmlview_jni_render_viewports(Client *client, float dtime);
	void htmlview_jni_poll(ScriptApiHTMLView *script);

#endif
