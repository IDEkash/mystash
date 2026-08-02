// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "guiBox.h"
#include <IVideoDriver.h>
#include "irr_v2d.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GUIBox::GUIBox(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
	const core::rect<s32> &rectangle,
	const std::array<video::SColor, 4> &colors,
	const std::array<video::SColor, 4> &bordercolors,
	const std::array<s32, 4> &borderwidths,
	s32 border_radius) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_colors(colors),
	m_bordercolors(bordercolors),
	m_borderwidths(borderwidths),
	m_border_radius(border_radius)
{
}

void GUIBox::draw()
{
	if (!IsVisible)
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	v2s32 upperleft = AbsoluteRect.UpperLeftCorner;
	v2s32 lowerright = AbsoluteRect.LowerRightCorner;

	s32 width = AbsoluteRect.getWidth();
	s32 height = AbsoluteRect.getHeight();
	s32 r = m_border_radius;

	// Clamp radius to half of width/height
	if (r > width / 2) r = width / 2;
	if (r > height / 2) r = height / 2;

	if (r <= 0) {
		std::array<s32, 4> negative_borders = {0, 0, 0, 0};
		std::array<s32, 4> positive_borders = {0, 0, 0, 0};

		for (size_t i = 0; i <= 3; i++) {
			if (m_borderwidths[i] > 0)
				positive_borders[i] = m_borderwidths[i];
			else
				negative_borders[i] = m_borderwidths[i];
		}

		v2s32 topleft_border = {
			upperleft.X - positive_borders[3],
			upperleft.Y - positive_borders[0]
		};
		v2s32 topleft_rect = {
			upperleft.X - negative_borders[3],
			upperleft.Y - negative_borders[0]
		};

		v2s32 lowerright_border = {
			lowerright.X + positive_borders[1],
			lowerright.Y + positive_borders[2]
		};
		v2s32 lowerright_rect = {
			lowerright.X + negative_borders[1],
			lowerright.Y + negative_borders[2]
		};

		core::rect<s32> main_rect(
			topleft_rect.X,
			topleft_rect.Y,
			lowerright_rect.X,
			lowerright_rect.Y
		);

		std::array<core::rect<s32>, 4> border_rects;

		border_rects[0] = core::rect<s32>(
			topleft_border.X,
			topleft_border.Y,
			lowerright_border.X,
			topleft_rect.Y
		);

		border_rects[1] = core::rect<s32>(
			lowerright_rect.X,
			topleft_rect.Y,
			lowerright_border.X,
			lowerright_rect.Y
		);

		border_rects[2] = core::rect<s32>(
			topleft_border.X,
			lowerright_rect.Y,
			lowerright_border.X,
			lowerright_border.Y
		);

		border_rects[3] = core::rect<s32>(
			topleft_border.X,
			topleft_rect.Y,
			topleft_rect.X,
			lowerright_rect.Y
		);

		driver->draw2DRectangle(main_rect, m_colors[0], m_colors[1], m_colors[3],
			m_colors[2], &AbsoluteClippingRect);

		core::rect<s32> border_rect = core::rect<s32>(
			topleft_border.X,
			topleft_border.Y,
			lowerright_border.X,
			lowerright_border.Y
		);
		if(!isNotClipped()) {
			border_rect.clipAgainst(Parent->getAbsoluteClippingRect());
		}

		for (size_t i = 0; i <= 3; i++)
			driver->draw2DRectangle(m_bordercolors[i], border_rects[i],
					&border_rect);

		IGUIElement::draw();
		return;
	}

	// Dynamic Rounded Corner Box Drawing!
	// 1. Draw central vertical block
	core::rect<s32> center_v(upperleft.X + r, upperleft.Y, lowerright.X - r, lowerright.Y);
	driver->draw2DRectangle(center_v, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);

	// 2. Draw left-wing and right-wing blocks (excluding corner heights)
	core::rect<s32> wing_l(upperleft.X, upperleft.Y + r, upperleft.X + r, lowerright.Y - r);
	driver->draw2DRectangle(wing_l, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);

	core::rect<s32> wing_r(lowerright.X - r, upperleft.Y + r, lowerright.X, lowerright.Y - r);
	driver->draw2DRectangle(wing_r, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);

	// 3. Draw rounded corner approximation (sub-rectangles drawing a quarter circle for each corner)
	for (s32 step = 0; step < r; step++) {
		f32 angle = ((f32)step / (f32)r) * (M_PI / 2.0);
		s32 px = r - (s32)(cos(angle) * r);
		s32 py = r - (s32)(sin(angle) * r);

		// Top Left Corner
		core::rect<s32> tl(upperleft.X + px, upperleft.Y + py, upperleft.X + r, upperleft.Y + py + 1);
		driver->draw2DRectangle(tl, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);

		// Top Right Corner
		core::rect<s32> tr(lowerright.X - r, upperleft.Y + py, lowerright.X - px, upperleft.Y + py + 1);
		driver->draw2DRectangle(tr, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);

		// Bottom Left Corner
		core::rect<s32> bl(upperleft.X + px, lowerright.Y - py - 1, upperleft.X + r, lowerright.Y - py);
		driver->draw2DRectangle(bl, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);

		// Bottom Right Corner
		core::rect<s32> br(lowerright.X - r, lowerright.Y - py - 1, lowerright.X - px, lowerright.Y - py);
		driver->draw2DRectangle(br, m_colors[0], m_colors[0], m_colors[0], m_colors[0], &AbsoluteClippingRect);
	}

	IGUIElement::draw();
}
