// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiCheckbox.h"
#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIFont.h"
#include "guiUtil.h"
#include "porting.h"

GUICheckBox::GUICheckBox(bool checked, gui::IGUIEnvironment *environment, gui::IGUIElement *parent, s32 id, core::rect<s32> rectangle) :
		gui::IGUICheckBox(environment, parent, id, rectangle), Checked(checked)
{
	setTabStop(true);
	setTabOrder(-1);
}

void GUICheckBox::setChecked(bool checked)
{
	Checked = checked;
}

bool GUICheckBox::isChecked() const
{
	return Checked;
}

void GUICheckBox::setDrawBackground(bool draw)
{
	Background = draw;
}

bool GUICheckBox::isDrawBackgroundEnabled() const
{
	return Background;
}

void GUICheckBox::setDrawBorder(bool draw)
{
	Border = draw;
}

bool GUICheckBox::isDrawBorderEnabled() const
{
	return Border;
}

bool GUICheckBox::OnEvent(const SEvent &event)
{
	if (isEnabled()) {
		switch (event.EventType) {
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown &&
					(event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE)) {
				Pressed = true;
				setFromState();
				return true;
			} else if (Pressed && event.KeyInput.PressedDown && event.KeyInput.Key == KEY_ESCAPE) {
				Pressed = false;
				setFromState();
				return true;
			} else if (!event.KeyInput.PressedDown && Pressed &&
					   (event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE)) {
				Pressed = false;
				setFromState();
				if (Parent) {
					SEvent newEvent;
					newEvent.EventType = EET_GUI_EVENT;
					newEvent.GUIEvent.Caller = this;
					newEvent.GUIEvent.Element = 0;
					Checked = !Checked;
					newEvent.GUIEvent.EventType = gui::EGET_CHECKBOX_CHANGED;
					Parent->OnEvent(newEvent);
				}
				return true;
			}
			break;
		case EET_GUI_EVENT:
			if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST) {
				if (event.GUIEvent.Caller == this) {
					Pressed = false;
					setFromState();
				}
			} else if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUSED) {
				if (event.GUIEvent.Caller == this) {
					setFromState();
				}
			} else if (event.GUIEvent.EventType == gui::EGET_ELEMENT_HOVERED || event.GUIEvent.EventType == gui::EGET_ELEMENT_LEFT) {
				setFromState();
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
				Pressed = true;
				CheckTime = (u32)porting::getTimeMs();
				setFromState();
				return true;
			} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
				bool wasPressed = Pressed;
				Pressed = false;
				setFromState();

				if (wasPressed && Parent) {
					if (!AbsoluteClippingRect.isPointInside(core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
						return true;
					}

					SEvent newEvent;
					newEvent.EventType = EET_GUI_EVENT;
					newEvent.GUIEvent.Caller = this;
					newEvent.GUIEvent.Element = 0;
					Checked = !Checked;
					newEvent.GUIEvent.EventType = gui::EGET_CHECKBOX_CHANGED;
					Parent->OnEvent(newEvent);
				}

				return true;
			}
			break;
		default:
			break;
		}
	}

	return gui::IGUIElement::OnEvent(event);
}

void GUICheckBox::draw()
{
	if (!IsVisible)
		return;

	gui::IGUISkin *skin = Environment->getSkin();
	if (skin) {
		video::IVideoDriver *driver = Environment->getVideoDriver();
		core::rect<s32> frameRect(AbsoluteRect);

		const s32 height = skin->getSize(gui::EGDS_CHECK_BOX_WIDTH);

		// draw background
		if (Background) {
			video::SColor bgColor = skin->getColor(gui::EGDC_3D_FACE);
			driver->draw2DRectangle(bgColor, frameRect, &AbsoluteClippingRect);
		}

		// draw the border
		if (Border) {
			skin->draw3DSunkenPane(this, 0, true, false, frameRect, &AbsoluteClippingRect);
		}

		// the rectangle around the "checked" area.
		core::rect<s32> checkRect(frameRect.UpperLeftCorner.X,
				((frameRect.getHeight() - height) / 2) + frameRect.UpperLeftCorner.Y,
				0, 0);

		checkRect.LowerRightCorner.X = checkRect.UpperLeftCorner.X + height;
		checkRect.LowerRightCorner.Y = checkRect.UpperLeftCorner.Y + height;

		if (BorderRadius > 0.0f) {
			gui::EGUI_DEFAULT_COLOR col = gui::EGDC_GRAY_EDITABLE;
			if (isEnabled())
				col = Pressed ? gui::EGDC_FOCUSED_EDITABLE : gui::EGDC_EDITABLE;
			gui::drawRoundedRectangle(driver, checkRect, skin->getColor(col), BorderRadius, &AbsoluteClippingRect);
		} else {
			gui::EGUI_DEFAULT_COLOR col = gui::EGDC_GRAY_EDITABLE;
			if (isEnabled())
				col = Pressed ? gui::EGDC_FOCUSED_EDITABLE : gui::EGDC_EDITABLE;
			skin->draw3DSunkenPane(this, skin->getColor(col),
					false, true, checkRect, &AbsoluteClippingRect);
		}

		// the checked icon
		if (Checked) {
			skin->drawIcon(this, gui::EGDI_CHECK_BOX_CHECKED, checkRect.getCenter(),
					CheckTime, (u32)porting::getTimeMs(), false, &AbsoluteClippingRect);
		}

		// associated text
		if (Text.size()) {
			checkRect = frameRect;
			checkRect.UpperLeftCorner.X += height + 5;

			gui::IGUIFont *font = skin->getFont();
			if (font) {
				font->draw(Text.c_str(), checkRect,
						skin->getColor(isEnabled() ? gui::EGDC_BUTTON_TEXT : gui::EGDC_GRAY_TEXT), false, true, &AbsoluteClippingRect);
			}
		}
	}
	gui::IGUIElement::draw();
}

void GUICheckBox::setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES> &styles)
{
	Styles = styles;
	setFromState();
}

void GUICheckBox::setFromState()
{
	StyleSpec::State state = StyleSpec::STATE_DEFAULT;
	if (Pressed)
		state = static_cast<StyleSpec::State>(state | StyleSpec::STATE_PRESSED);
	if (Environment->getHovered() == this)
		state = static_cast<StyleSpec::State>(state | StyleSpec::STATE_HOVERED);
	if (Environment->hasFocus(this))
		state = static_cast<StyleSpec::State>(state | StyleSpec::STATE_FOCUSED);

	setFromStyle(StyleSpec::getStyleFromStatePropagation(Styles, state));
}

void GUICheckBox::setFromStyle(const StyleSpec &style)
{
	BorderRadius = style.getFloat(StyleSpec::BORDER_RADIUS, 0.0f);
	// More styling can be added here
}

GUICheckBox* GUICheckBox::addCheckBox(gui::IGUIEnvironment *environment, bool checked,
		const core::rect<s32>& rectangle, gui::IGUIElement* parent, s32 id, const wchar_t* text)
{
	GUICheckBox* b = new GUICheckBox(checked, environment, parent ? parent : environment->getRootGUIElement(), id, rectangle);
	if (text)
		b->setText(text);
	b->drop();
	return b;
}
