// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiProgressBar.h"
#include <IVideoDriver.h>

GUIProgressBar::GUIProgressBar(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
	const core::rect<s32> &rectangle, f32 value, f32 max, bool vertical) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_value(value), m_max(max), m_vertical(vertical)
{
}

void GUIProgressBar::draw()
{
	if (!IsVisible)
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	video::SColor bgcolor = m_style.getColor(StyleSpec::BGCOLOR, video::SColor(255, 50, 50, 50));
	video::SColor fgcolor = m_style.getColor(StyleSpec::COLORS, video::SColor(255, 0, 200, 0));

	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	core::rect<s32> fill_rect = AbsoluteRect;
	f32 ratio = (m_max > 0) ? (m_value / m_max) : 0;
	ratio = core::clamp(ratio, 0.0f, 1.0f);

	if (m_vertical) {
		fill_rect.UpperLeftCorner.Y = fill_rect.LowerRightCorner.Y - (s32)(AbsoluteRect.getHeight() * ratio);
	} else {
		fill_rect.LowerRightCorner.X = fill_rect.UpperLeftCorner.X + (s32)(AbsoluteRect.getWidth() * ratio);
	}

	driver->draw2DRectangle(fgcolor, fill_rect, &AbsoluteClippingRect);

	IGUIElement::draw();
}
