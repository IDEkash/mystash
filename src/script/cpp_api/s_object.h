// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "cpp_api/s_base.h"
#include "irr_v3f.h"
#include "tool.h"

class ScriptApiObject : virtual public ScriptApiBase
{
public:
	void sceneobject_step(u16 id, float dtime);
	void sceneobject_on_punch(u16 id, ServerActiveObject *puncher,
			float time_from_last_punch, const ToolCapabilities &toolcap, v3f dir);
	void sceneobject_on_rightclick(u16 id, ServerActiveObject *clicker);
};
