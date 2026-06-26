// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "IGUICheckBox.h"
#include "StyleSpec.h"
#include <array>

class GUICheckBox : public gui::IGUICheckBox
{
public:
	GUICheckBox(bool checked, gui::IGUIEnvironment* environment, gui::IGUIElement* parent, s32 id, core::rect<s32> rectangle);

	virtual void setChecked(bool checked) override;
	virtual bool isChecked() const override;
	virtual void setDrawBackground(bool draw) override;
	virtual bool isDrawBackgroundEnabled() const override;
	virtual void setDrawBorder(bool draw) override;
	virtual bool isDrawBorderEnabled() const override;

	virtual bool OnEvent(const SEvent& event) override;
	virtual void draw() override;

	void setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES>& styles);

	static GUICheckBox* addCheckBox(gui::IGUIEnvironment *environment, bool checked,
			const core::rect<s32>& rectangle, gui::IGUIElement* parent, s32 id, const wchar_t* text);

private:
	void setFromState();
	void setFromStyle(const StyleSpec& style);

	u32 CheckTime = 0;
	bool Pressed = false;
	bool Checked;
	bool Border = false;
	bool Background = false;

	std::array<StyleSpec, StyleSpec::NUM_STATES> Styles;
	float BorderRadius = 0.0f;
};
