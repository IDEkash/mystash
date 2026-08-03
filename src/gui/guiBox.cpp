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
	s32 bw = m_borderwidths[0]; // Uniform border width

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

	// Dynamic Rounded Corner Box and Border Drawing with NO bleed-through, NO gaps, NO cos/sin!
	video::SColor c_bg = m_colors[0];
	video::SColor c_border = m_bordercolors[0];

	if (bw > r) bw = r; // Clamp border width to radius

	// 1. Draw central vertical block
	if (bw > 0) {
		// Top border of central block
		core::rect<s32> top_border(upperleft.X + r, upperleft.Y, lowerright.X - r, upperleft.Y + bw);
		driver->draw2DRectangle(top_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);

		// Center fill of central block
		core::rect<s32> center_fill(upperleft.X + r, upperleft.Y + bw, lowerright.X - r, lowerright.Y - bw);
		driver->draw2DRectangle(center_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);

		// Bottom border of central block
		core::rect<s32> bottom_border(upperleft.X + r, lowerright.Y - bw, lowerright.X - r, lowerright.Y);
		driver->draw2DRectangle(bottom_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);
	} else {
		core::rect<s32> center_v(upperleft.X + r, upperleft.Y, lowerright.X - r, lowerright.Y);
		driver->draw2DRectangle(center_v, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
	}

	// 2. Draw left-wing and right-wing blocks (excluding corner heights)
	if (bw > 0) {
		// Left border
		core::rect<s32> left_border(upperleft.X, upperleft.Y + r, upperleft.X + bw, lowerright.Y - r);
		driver->draw2DRectangle(left_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);

		// Left center fill
		core::rect<s32> left_fill(upperleft.X + bw, upperleft.Y + r, upperleft.X + r, lowerright.Y - r);
		driver->draw2DRectangle(left_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);

		// Right border
		core::rect<s32> right_border(lowerright.X - bw, upperleft.Y + r, lowerright.X, lowerright.Y - r);
		driver->draw2DRectangle(right_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);

		// Right center fill
		core::rect<s32> right_fill(lowerright.X - r, upperleft.Y + r, lowerright.X - bw, lowerright.Y - r);
		driver->draw2DRectangle(right_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
	} else {
		core::rect<s32> wing_l(upperleft.X, upperleft.Y + r, upperleft.X + r, lowerright.Y - r);
		driver->draw2DRectangle(wing_l, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);

		core::rect<s32> wing_r(lowerright.X - r, upperleft.Y + r, lowerright.X, lowerright.Y - r);
		driver->draw2DRectangle(wing_r, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
	}

	// 3. Draw rounded corners disjointly row-by-row (exact-circle, no gaps, no cos/sin!)
	s32 r_inner = r - bw;
	for (s32 y = 0; y < r; y++) {
		// Outer boundary X
		s32 x_outer = r - (s32)std::round(std::sqrt((f32)(r * r - y * y)));

		// Inner boundary X
		s32 x_inner = r;
		if (bw > 0) {
			s32 y_inner = y - bw;
			if (y_inner >= 0 && r_inner > 0) {
				x_inner = bw + r_inner - (s32)std::round(std::sqrt((f32)(r_inner * r_inner - y_inner * y_inner)));
			}
		}

		// Row segments for Top Left Corner
		if (x_inner > x_outer) {
			core::rect<s32> tl_border(upperleft.X + x_outer, upperleft.Y + y, upperleft.X + x_inner, upperleft.Y + y + 1);
			driver->draw2DRectangle(tl_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);
		}
		if (r > x_inner) {
			core::rect<s32> tl_fill(upperleft.X + x_inner, upperleft.Y + y, upperleft.X + r, upperleft.Y + y + 1);
			driver->draw2DRectangle(tl_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
		}

		// Row segments for Top Right Corner
		if (x_inner > x_outer) {
			core::rect<s32> tr_border(lowerright.X - x_inner, upperleft.Y + y, lowerright.X - x_outer, upperleft.Y + y + 1);
			driver->draw2DRectangle(tr_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);
		}
		if (r > x_inner) {
			core::rect<s32> tr_fill(lowerright.X - r, upperleft.Y + y, lowerright.X - x_inner, upperleft.Y + y + 1);
			driver->draw2DRectangle(tr_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
		}

		// Row segments for Bottom Left Corner
		if (x_inner > x_outer) {
			core::rect<s32> bl_border(upperleft.X + x_outer, lowerright.Y - y - 1, upperleft.X + x_inner, lowerright.Y - y);
			driver->draw2DRectangle(bl_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);
		}
		if (r > x_inner) {
			core::rect<s32> bl_fill(upperleft.X + x_inner, lowerright.Y - y - 1, upperleft.X + r, lowerright.Y - y);
			driver->draw2DRectangle(bl_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
		}

		// Row segments for Bottom Right Corner
		if (x_inner > x_outer) {
			core::rect<s32> br_border(lowerright.X - x_inner, lowerright.Y - y - 1, lowerright.X - x_outer, lowerright.Y - y);
			driver->draw2DRectangle(br_border, c_border, c_border, c_border, c_border, &AbsoluteClippingRect);
		}
		if (r > x_inner) {
			core::rect<s32> br_fill(lowerright.X - r, lowerright.Y - y - 1, lowerright.X - x_inner, lowerright.Y - y);
			driver->draw2DRectangle(br_fill, c_bg, c_bg, c_bg, c_bg, &AbsoluteClippingRect);
		}
	}

	IGUIElement::draw();
}
