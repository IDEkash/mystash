// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#include "guiNativeUI.h"
#include "guiButton.h"
#include "guiBox.h"
#include "client/texturepaths.h"
#include <IGUIStaticText.h>
#include <IGUIImage.h>
#include "log.h"
#include "util/string.h"
#include "scripting_mainmenu.h"

GUINativeUI::GUINativeUI(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id,
						 IMenuManager *menumgr, ISimpleTextureSource *tsrc, MainMenuScripting *script) :
	GUIModalMenu(env, parent, id, menumgr),
	m_tsrc(tsrc),
	m_script(script)
{
}

GUINativeUI::~GUINativeUI()
{
	clearWidgets();
}

void GUINativeUI::loadLayout(const Json::Value &layout)
{
	m_layout_data = layout;
	regenerateGui(m_screensize_old);
}

void GUINativeUI::regenerateGui(v2u32 screensize)
{
	m_screensize_old = screensize;
	clearWidgets();

	if (m_layout_data.isNull())
		return;

	buildUI(m_layout_data, this);
}

void GUINativeUI::drawMenu()
{
	video::IVideoDriver* driver = Environment->getVideoDriver();
	driver->draw2DRectangle(video::SColor(128, 0, 0, 0), AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

bool GUINativeUI::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			std::string id = getWidgetID(event.GUIEvent.Caller);
			if (!id.empty() && m_script) {
				m_script->handleNativeEvent(id, "click");
			}
			return true;
		}
	}
	return GUIModalMenu::OnEvent(event);
}

gui::IGUIElement* GUINativeUI::getWidget(const std::string &id)
{
	auto it = m_widgets.find(id);
	if (it != m_widgets.end())
		return it->second;
	return nullptr;
}

std::string GUINativeUI::getWidgetID(gui::IGUIElement* element)
{
	for (auto const& [id, widget] : m_widgets) {
		if (widget == element)
			return id;
	}
	return "";
}

void GUINativeUI::clearWidgets()
{
	m_widgets.clear();
	Environment->setFocus(nullptr);

	// Remove all children
	const std::list<gui::IGUIElement*>& children = getChildren();
	std::list<gui::IGUIElement*> to_remove;
	for (gui::IGUIElement* child : children) {
		to_remove.push_back(child);
	}
	for (gui::IGUIElement* child : to_remove) {
		child->remove();
	}
}

void GUINativeUI::buildUI(const Json::Value &data, gui::IGUIElement *parent)
{
	parseWidget(data, parent);
}

void GUINativeUI::parseWidget(const Json::Value &value, gui::IGUIElement *parent)
{
	std::string type = value.get("type", "").asString();
	std::string id = value.get("id", "").asString();

	gui::IGUIElement* element = nullptr;

	core::rect<s32> rect = parseRect(value["position"], value["size"]);

	if (type == "panel") {
		video::SColor color(255, 255, 255, 255);
		if (value.isMember("color")) {
			parseColorString(value["color"].asString(), color, true);
		}
		std::array<video::SColor, 4> colors;
		colors.fill(color);
		std::array<video::SColor, 4> bordercolors;
		bordercolors.fill(video::SColor(0,0,0,0));
		std::array<s32, 4> borderwidths;
		borderwidths.fill(0);
		element = new GUIBox(Environment, parent, -1, rect, colors, bordercolors, borderwidths);
	} else if (type == "text") {
		std::wstring text = utf8_to_wide(value.get("value", "").asString());
		element = Environment->addStaticText(text.c_str(), rect, false, true, parent);
		if (element && value.isMember("font_size")) {
			// Font size handling would go here, Irrlicht font system is a bit complex for a quick fix
		}
	} else if (type == "button") {
		std::wstring label = utf8_to_wide(value.get("label", "").asString());
		element = GUIButton::addButton(Environment, rect, m_tsrc, parent, -1, label.c_str());
	} else if (type == "image") {
		std::string texture_name = value.get("value", "").asString();
		video::ITexture* tex = m_tsrc->getTexture(texture_name);
		element = Environment->addImage(tex, rect.UpperLeftCorner, true, parent);
		if (element) {
			element->setRelativePosition(rect);
			static_cast<gui::IGUIImage*>(element)->setScaleImage(true);
		}
	} else if (type == "container") {
		element = new gui::IGUIElement(gui::EGUIET_ELEMENT, Environment, parent, -1, rect);
	}

	if (element) {
		if (!id.empty()) {
			m_widgets[id] = element;
		}

		if (value.isMember("children") && value["children"].isArray()) {
			for (const auto& child : value["children"]) {
				parseWidget(child, element);
			}
		}
	}
}

core::rect<s32> GUINativeUI::parseRect(const Json::Value &pos, const Json::Value &size)
{
	s32 x = 0, y = 0, w = 100, h = 100;
	if (pos.isArray() && pos.size() >= 2) {
		x = pos[0].asInt();
		y = pos[1].asInt();
	}
	if (size.isArray() && size.size() >= 2) {
		w = size[0].asInt();
		h = size[1].asInt();
	}
	return core::rect<s32>(x, y, x + w, y + h);
}
