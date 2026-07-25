-- Luanti
-- Copyright (C) 2025 Jules
-- SPDX-License-Identifier: LGPL-2.1-or-later

local function internal_consent_formspec(data)
	local mod_name = data.mod_name
	local content =
		fgettext("This mod contains '.int' or 'internal.init' files, opting out of the standard Luanti sandbox.\n\n" ..
		"• What 'internal' means:\n" ..
		"  This mod can read/write files on your device, potentially beyond the game's folder. " ..
		"It can directly affect game stability, making crashes more likely and harder to recover from, " ..
		"and can access physics/rendering internals standard mods cannot touch.\n\n" ..
		"• Why this is different:\n" ..
		"  Standard Lua mods run in a secure sandbox. Internal-access mods opt out of that sandbox. " ..
		"There is no per-mod review process checking what this specific mod does before you enable it.\n\n" ..
		"• What can go wrong:\n" ..
		"  A malicious or buggy internal mod could damage your save data, other files on your device, or more. " ..
		"Because there is no gatekeeper, your own judgment of the mod's source is the actual safeguard.\n\n" ..
		"• Practical guidance:\n" ..
		"  Only enable this if you fully trust the specific author/source. Check the mod's page/repository " ..
		"for reports of problems. Enabling this is NOT reversible in effect—once it runs, any damage " ..
		"done cannot be undone, even if you disable the mod later.\n\n" ..
		"Are you sure you want to proceed and enable this mod?")

	local formspec = {
		"formspec_version[10]",
		"size[11,8.5]",
		"bgcolor[;true]",
		"style_type[box;colors=#0f172a,#1e293b,#1e293b,#0f172a]",
		"box[-0.5,-0.5;12,9.5;]",
		"style_type[label;font=bold]",
		"style[title_lbl;font=bold;font_size=+14;textcolor=#ef4444]",
		"label[0.5,0.5;title_lbl;" .. fgettext("Warning: Elevated Permissions Required") .. "]",
		"style[sub_lbl;font_size=+11;textcolor=white]",
		"label[0.5,1.1;sub_lbl;" .. fgettext_ne("The mod \"$1\" requests internal engine access.", mod_name) .. "]",
		"textarea[0.5,1.7;10.0,5.0;;;" .. core.formspec_escape(content) .. "]",
		"style[btn_proceed;bgcolor=#ef4444;textcolor=white;font=bold;border=false]",
		"style[btn_proceed:hovered;bgcolor=#dc2626]",
		"style[btn_proceed:pressed;bgcolor=#991b1b]",
		"button[5.3,7.5;2.5,0.7;btn_proceed;" .. fgettext("Proceed") .. "]",
		"style[btn_cancel;bgcolor=#475569;textcolor=white;border=false]",
		"style[btn_cancel:hovered;bgcolor=#64748b]",
		"style[btn_cancel:pressed;bgcolor=#334155]",
		"button[8.0,7.5;2.5,0.7;btn_cancel;" .. fgettext("Cancel") .. "]",
	}
	return table.concat(formspec, "")
end

local function internal_consent_buttonhandler(this, fields)
	if fields.btn_proceed then
		this:delete()
		if this.data.on_accept then
			this.data.on_accept()
		end
		return true
	elseif fields.btn_cancel or fields.key_escape then
		this:delete()
		return true
	end
	return false
end

local function event_handler(event)
	if event == "DialogShow" then
		mm_game_theme.set_engine(true)
		return true
	end
	return false
end

function create_internal_consent_dialog(parent, on_accept, mod_name)
	local retval = dialog_create("dlg_internal_consent",
		internal_consent_formspec,
		internal_consent_buttonhandler,
		event_handler)
	retval.data.on_accept = on_accept
	retval.data.mod_name = mod_name
	return retval
end
