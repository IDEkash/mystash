// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#pragma once

#include "modalMenu.h"
#include "irrlichttypes_bloated.h"
#include <string>
#include <unordered_map>
#include <list>
#include <array>
#include <json/json.h>

class ISimpleTextureSource;

class MainMenuScripting;

class GUINativeUI : public GUIModalMenu
{
public:
	GUINativeUI(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id,
				IMenuManager *menumgr, ISimpleTextureSource *tsrc, MainMenuScripting *script);
	virtual ~GUINativeUI();

	void loadLayout(const Json::Value &layout);

	// GUIModalMenu implementation
	void regenerateGui(v2u32 screensize) override;
	void drawMenu() override;
	bool OnEvent(const SEvent &event) override;

	// Widget access and manipulation
	gui::IGUIElement* getWidget(const std::string &id);
	std::string getWidgetID(gui::IGUIElement* element);

protected:
	std::wstring getLabelByID(s32 id) override { return L""; }
	std::string getNameByID(s32 id) override { return ""; }

private:
	ISimpleTextureSource *m_tsrc;
	MainMenuScripting *m_script;
	std::unordered_map<std::string, gui::IGUIElement*> m_widgets;
	Json::Value m_layout_data;

	void clearWidgets();
	void buildUI(const Json::Value &data, gui::IGUIElement *parent);
	void parseWidget(const Json::Value &value, gui::IGUIElement *parent);

	core::rect<s32> parseRect(const Json::Value &pos, const Json::Value &size);
};
