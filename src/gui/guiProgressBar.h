// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <IGUIElement.h>
#include <IGUIEnvironment.h>
#include "StyleSpec.h"

class GUIProgressBar : public gui::IGUIElement
{
public:
	GUIProgressBar(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const core::rect<s32> &rectangle, f32 value, f32 max, bool vertical);

	virtual void draw() override;

	void setValue(f32 value) { m_value = value; }
	void setStyle(const StyleSpec &style) { m_style = style; }

private:
	f32 m_value;
	f32 m_max;
	bool m_vertical;
	StyleSpec m_style;
};
