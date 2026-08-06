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
			"<tag name=heading color=#ff0>",
			"<tag name=gray color=#aaa>",
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

		local MARGIN = 0.5
		local LEFT_W = 5
		local RIGHT_X = MARGIN + LEFT_W + 0.5
		local RIGHT_W = tabview.width - RIGHT_X - MARGIN

		local fs = "container[" .. MARGIN .. "," .. MARGIN .. "]" ..
			"image[1,0.5;3,3;" .. core.formspec_escape(logofile) .. "]" ..
			"label[0,4.2;" .. core.colorize("#fff", core.formspec_escape(version.project .. " " .. version.string)) .. "]" ..
			"style[homepage;bgcolor=#43464b;textcolor=white]" ..
			"button_url[0,4.8;5,0.8;homepage;www.luanti.org;https://www.luanti.org/]" ..
			"tooltip[homepage;" .. fgettext("Visit the official website") .. "]"

		local active_renderer_info = fgettext("Active renderer:") .. " " .. core.formspec_escape(get_renderer_info())
		fs = fs .. "label[0,6;" .. core.colorize("#aaa", active_renderer_info) .. "]"

		if PLATFORM == "Android" then
			fs = fs .. "style[share_debug;bgcolor=#43464b;textcolor=white]" ..
				"button[0,6.5;5,0.8;share_debug;" .. fgettext("Share debug log") .. "]" ..
				"tooltip[share_debug;" .. fgettext("Share the debug log for troubleshooting") .. "]"
		else
			fs = fs .. "style[userdata;bgcolor=#43464b;textcolor=white]" ..
				"button[0,6.5;5,0.8;userdata;" .. fgettext("Open User Data Directory") .. "]" ..
				"tooltip[userdata;" .. fgettext("Open folder in file manager") .. "]"
		end
		fs = fs .. "container_end[]"

		fs = fs .. "container[" .. RIGHT_X .. "," .. MARGIN .. "]" ..
			"box[0,0;" .. RIGHT_W .. ",6.6;#00000040]" ..
			"hypertext[0.1,0.1;" .. (RIGHT_W-0.2) .. ",6.4;credits;" .. core.formspec_escape(hypertext) .. "]" ..
			"container_end[]"

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
