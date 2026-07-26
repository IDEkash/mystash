-- Luanti
-- Copyright (C) 2025 siliconsniffer
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function exit_dialog_formspec()
	local show_dialog = core.settings:get_bool("enable_esc_dialog", true)
	local formspec = {
		"formspec_version[10]" ..
		"size[10,3.6]" ..
		"style_type[label;font=bold]" ..
		"style[btn_quit_confirm_yes;bgcolor=#9b2c2c;textcolor=white]" ..
		"style[btn_quit_confirm_yes:hovered;bgcolor=#b91c1c]" ..
		"style[btn_quit_confirm_cancel;bgcolor=#334155;textcolor=white]" ..
		"style[btn_quit_confirm_cancel:hovered;bgcolor=#475569]" ..

		-- Solid modern greyish-blue background (Layer 1)
		"box[-0.5,-0.5;11.0,4.6;#0f172aF2]" ..

		-- Rounded Container Card (Layer 2)
		"background9[0.15,0.15;9.7;3.3;button_hover_semitrans.png;false;6,6]" ..

		"label[0.5,0.5;" .. fgettext("Are you sure you want to quit?") .. "]" ..
		"checkbox[0.5,1.4;cb_show_dialog;" .. fgettext("Always show this dialog.") .. ";" .. tostring(show_dialog) .. "]" ..
		"button[0.5,2.3;3,0.8;btn_quit_confirm_cancel;" .. fgettext("Cancel") .. "]" ..
		"button[6.5,2.3;3,0.8;btn_quit_confirm_yes;" .. fgettext("Quit") .. "]" ..
		"set_focus[btn_quit_confirm_yes]"
	}
	return table.concat(formspec, "")
end


local function exit_dialog_buttonhandler(this, fields)
	if fields.cb_show_dialog ~= nil then
		core.settings:set_bool("enable_esc_dialog", core.is_yes(fields.cb_show_dialog))
		return false
	elseif fields.btn_quit_confirm_yes then
		this:delete()
		core.close()
		return true
	elseif fields.btn_quit_confirm_cancel then
		this:delete()
		this:show()
		return true
	end
end


local function event_handler(event)
	if event == "DialogShow" then
		mm_game_theme.set_engine(true) -- hide the menu header
		return true
	end
	return false
end


function create_exit_dialog()
	local retval = dialog_create("dlg_exit",
		exit_dialog_formspec,
		exit_dialog_buttonhandler,
		event_handler)
	return retval
end
