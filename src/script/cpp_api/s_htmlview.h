// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "cpp_api/s_base.h"

#include <string>

class ScriptApiHTMLView : virtual public ScriptApiBase
{
public:
	bool on_htmlview_message(const std::string &id, const std::string &message);
	bool on_htmlview_capture(const std::string &id, const std::string &png_base64);
};
