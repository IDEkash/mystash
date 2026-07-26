-- Luanti
-- Copyright (C) 2024 siliconsniffer
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function clients_list_formspec(dialogdata)
	local TOUCH_GUI = core.settings:get_bool("touch_gui")
	local clients_list = dialogdata.server.clients_list
	local servername   = dialogdata.server.name

	local function fmt_formspec_list(clients_list)
		local escaped = {}
		for i, str in ipairs(clients_list) do
			escaped[i] = core.formspec_escape(str)
		end
		return table.concat(escaped, ",")
	end

	local formspec = {
		"formspec_version[10]",
		"size[6,9.5]",
		TOUCH_GUI and "padding[0.01,0.01]" or "",
		"style[quit;bgcolor=#0284c7;textcolor=white;font=bold]",
		"style[quit:hovered;bgcolor=#0369a1]",
		"style_type[textlist;bgcolor=#1e293b;textcolor=white;border=true;border_color=#334155]",

		-- Solid modern greyish-blue background (Layer 1)
		"box[-0.5,-0.5;7.0,10.5;#0f172aF2]",

		-- Rounded Container Card (Layer 2)
		"background9[0.15,0.15;5.7;9.2;button_hover_semitrans.png;false;6,6]",

		"hypertext[0,0.2;6,1.5;;<global margin=5 halign=center valign=middle color=#ffffff>",
			-- TRANSLATORS: $1 = server name
			fgettext("Players connected to\n$1",
				"<b><style color=#38bdf8>" .. core.hypertext_escape(servername) .. "</style></b>") .. "]",
		"textlist[0.5,1.7;5,6.5;;" .. fmt_formspec_list(clients_list) .. "]",
		"button[1.5,8.4;3,0.8;quit;OK]"
	}
	return table.concat(formspec, "")
end


local function clients_list_buttonhandler(this, fields)
	if fields.quit then
		this:delete()
		return true
	end
	return false
end


function create_clientslist_dialog(server)
	local retval = dialog_create("dlg_clients_list",
		clients_list_formspec,
		clients_list_buttonhandler,
		nil)
	retval.data.server = server
	return retval
end
