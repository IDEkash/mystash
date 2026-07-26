-- Luanti
-- Copyright (C) 2013 sapier
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function prepare_credits(dest, source)
	local string = table.concat(source, "\n") .. "\n"

	string = core.hypertext_escape(string)
	string = string:gsub("%[.-%]", "<gray>%1</gray>")

	table.insert(dest, string)
end

local function get_credits()
	local f = assert(io.open(core.get_mainmenu_path() .. "/credits.json"))
	local json = core.parse_json(f:read("*all"))
	f:close()
	return json
end

local function get_renderer_info()
	local ret = {}

	-- OpenGL version, stripped to just the important part
	local s1 = core.get_active_renderer()
	if s1:sub(1, 7) == "OpenGL " then
		s1 = s1:sub(8)
	end
	local m = s1:match("^[%d.]+")
	if not m then
		m = s1:match("^ES [%d.]+")
	end
	ret[#ret+1] = m or s1
	-- video driver
	ret[#ret+1] = core.get_active_driver():lower()
	-- irrlicht device
	ret[#ret+1] = core.get_active_irrlicht_device():upper()

	return table.concat(ret, " / ")
end

return {
	name = "about",
	caption = fgettext("About"),

	cbf_formspec = function(tabview, name, tabdata)
		local logofile = defaulttexturedir .. "logo.png"
		local version = core.get_version()

		local hypertext = {
			"<tag name=heading color=#38bdf8 font=bold size=18>",
			"<tag name=gray color=#94a3b8>",
		}

		local credits = get_credits()

		table.insert_all(hypertext, {
			"<heading>", fgettext_ne("Core Developers"), "</heading>\n",
		})
		prepare_credits(hypertext, credits.core_developers)
		table.insert_all(hypertext, {
			"\n",
			"<heading>", fgettext_ne("Core Team"), "</heading>\n",
		})
		prepare_credits(hypertext, credits.core_team)
		table.insert_all(hypertext, {
			"\n",
			"<heading>", fgettext_ne("Active Contributors"), "</heading>\n",
		})
		prepare_credits(hypertext, credits.contributors)
		table.insert_all(hypertext, {
			"\n",
			"<heading>", fgettext_ne("Previous Core Developers"), "</heading>\n",
		})
		prepare_credits(hypertext, credits.previous_core_developers)
		table.insert_all(hypertext, {
			"\n",
			"<heading>", fgettext_ne("Previous Contributors"), "</heading>\n",
		})
		prepare_credits(hypertext, credits.previous_contributors)

		hypertext = table.concat(hypertext):sub(1, -2)

		-- Premium Modern Grayish Blue styling with background9 double panels
		local fs = "style[label_button;border=false;font=bold;textcolor=#38bdf8]" ..
			"style[label_button2;border=false;textcolor=#94a3b8]" ..
			"style[homepage;bgcolor=#0284c7;textcolor=white;font=bold]" ..
			"style[homepage:hovered;bgcolor=#0369a1]" ..
			"style[userdata,share_debug;bgcolor=#334155;textcolor=white]" ..
			"style[userdata:hovered,share_debug:hovered;bgcolor=#475569]" ..
			"background9[0.15,0.15;5.1,6.8;button_hover_semitrans.png;false;6,6]" ..
			"background9[5.4,0.15;10.1,6.8;button_hover_semitrans.png;false;6,6]" ..

			"image[1.45,0.4;2.5,2.5;" .. core.formspec_escape(logofile) .. "]" ..
			"button[0.25,3.1;4.9,0.5;label_button;" ..
			core.formspec_escape(version.project .. " " .. version.string) .. "]" ..
			"button_url[1.45,3.75;2.5,0.65;homepage;luanti.org;https://www.luanti.org/]" ..
			"hypertext[5.65,0.4;9.6;6.3;credits;" .. core.formspec_escape(hypertext) .. "]"

		local active_renderer_info = fgettext("Active renderer:") .. "\n" ..
			core.formspec_escape(get_renderer_info())
		fs = fs .. "button[0.25,5.5;4.9,1.1;label_button2;" .. active_renderer_info .. "]"..
			"tooltip[label_button2;" .. active_renderer_info .. "]"

		if PLATFORM == "Android" then
			fs = fs .. "button[0.45,4.65;4.5,0.65;share_debug;" .. fgettext("Share debug log") .. "]"
		else
			fs = fs .. "tooltip[userdata;" ..
					fgettext("Opens the directory that contains user-provided worlds, games, mods,\n" ..
							"and texture packs in a file manager / explorer.") .. "]"
			fs = fs .. "button[0.45,4.65;4.5,0.65;userdata;" .. fgettext("Open User Data Directory") .. "]"
		end

		return fs
	end,

	cbf_button_handler = function(this, fields, name, tabdata)
		if fields.share_debug then
			local path = core.get_user_path() .. DIR_DELIM .. "debug.txt"
			core.share_file(path)
		end

		if fields.userdata then
			core.open_dir(core.get_user_path())
		end
	end,

	on_change = function(type)
		if type == "ENTER" then
			mm_game_theme.set_engine()
		end
	end,
}
