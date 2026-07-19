-- Luanti
-- Copyright (C) 2013 sapier
-- SPDX-License-Identifier: LGPL-2.1-or-later

--------------------------------------------------------------------------------

local enabled_all = false

local function modname_valid(name)
	return not name:find("[^a-z0-9_]")
end

local function init_data(data)
	data.list = filterlist.create(
		pkgmgr.preparemodlist,
		pkgmgr.comparemod,
		function(element, uid)
			if element.name == uid then
				return true
			end
		end,
		nil,
		{
			worldpath = data.worldspec.path,
			gameid = data.worldspec.gameid
		}
	)

	if data.selected_mod > data.list:size() then
		data.selected_mod = 0
	end

	-- Sorting is already done by pgkmgr.get_mods
end


-- Returns errors errors and a list of all enabled mods (inc. game and world mods)
--
-- `with_errors` is a table from mod virtual path to `{ type = "error" | "warning" }`.
-- `enabled_mods_by_name` is a table from mod virtual path to `true`.
--
-- @param world_path Path to the world
-- @param all_mods List of mods, with `enabled` property.
-- @returns with_errors, enabled_mods_by_name
local function check_mod_configuration(world_path, all_mods)
	-- Build up lookup tables for enabled mods and all mods by vpath
	local enabled_mod_paths = {}
	local all_mods_by_vpath = {}
	for _, mod in ipairs(all_mods)  do
		if mod.type == "mod" then
			all_mods_by_vpath[mod.virtual_path] = mod
		end
		if mod.enabled then
			enabled_mod_paths[mod.virtual_path] = mod.path
		end
	end

	-- Use the engine's mod configuration code to resolve dependencies and return any errors
	local config_status = core.check_mod_configuration(world_path, enabled_mod_paths)

	-- Build the list of enabled mod virtual paths
	local enabled_mods_by_name = {}
	for _, mod in ipairs(config_status.satisfied_mods) do
		assert(mod.virtual_path ~= "")
		enabled_mods_by_name[mod.name] = all_mods_by_vpath[mod.virtual_path] or mod
	end
	for _, mod in ipairs(config_status.unsatisfied_mods) do
		assert(mod.virtual_path ~= "")
		enabled_mods_by_name[mod.name] = all_mods_by_vpath[mod.virtual_path] or mod
	end

	-- Build the table of errors
	local with_error = {}
	for _, mod in ipairs(config_status.unsatisfied_mods) do
		local error = { type = "warning" }
		with_error[mod.virtual_path] = error

		for _, depname in ipairs(mod.unsatisfied_depends) do
			if not enabled_mods_by_name[depname] then
				error.type = "error"
				break
			end
		end
	end

	return with_error, enabled_mods_by_name
end

local function get_formspec(data)
	if not data.list then
		init_data(data)
	end

	local all_mods = data.list:get_list()
	local with_error, enabled_mods_by_name = check_mod_configuration(data.worldspec.path, all_mods)

	local mod = all_mods[data.selected_mod] or {name = ""}

	-- Core premium greyish blue Slate container setup - Expanded height and width for generous spacing (Re-skin to dark marketplace)
	local retval =
		"formspec_version[7]" ..
		"size[12.0,9.0]" ..
		"padding[0.15,0.15]" ..
		"bgcolor[;true]" ..
		"box[-0.5,-0.5;13.0,10.0;#000000]" .. -- Root #000000

		-- Title Header Area
		"style[title_lbl;font=bold;font_size=+14;textcolor=#f2f2f4]" ..
		"label[0.3,0.3;title_lbl;" .. fgettext("Configure Mods for:") .. "]" ..
		"style[world_lbl;font=bold;font_size=+14;textcolor=#3a7bfd]" ..
		"label[4.2,0.3;world_lbl;" .. core.formspec_escape(data.worldspec.name) .. "]" ..

		-- Left Pane Card (Layer 2) - flat fill #0d0d0f, border #232326
		"box[0.2,0.8;5.6,7.2;#0d0d0f]" ..
		"box[0.2,0.8;5.6,7.2;#232326;true]" ..
		"container[0.4,1.0]"

	if mod.is_modpack or mod.type == "game" then
		local info = core.formspec_escape(
			core.get_content_info(mod.path).description)
		if info == "" then
			if mod.is_modpack then
				info = fgettext("No modpack description provided.")
			else
				info = fgettext("No game description provided.")
			end
		end
		retval = retval ..
			"textarea[0,0;5.2,6.8;;" .. info .. ";]"
	elseif mod.type == "worldmods" then
		retval = retval ..
			"textarea[0,0;5.2,6.8;;" ..
			fgettext("Mods located inside the world folder.") .. ";]"
	else
		local hard_deps, soft_deps = pkgmgr.get_dependencies(mod.path)

		-- Add error messages to dep lists
		if mod.enabled or mod.always_on then
			for i, dep_name in ipairs(hard_deps) do
				local dep = enabled_mods_by_name[dep_name]
				if not dep then
					-- TRANSLATORS: Displayed when a mod dependency is unsatisfied (danger/sale #e34848)
					hard_deps[i] = minetest.colorize("#e34848", dep_name .. " " .. fgettext("(Unsatisfied)"))
				elseif with_error[dep.virtual_path] then
					-- TRANSLATORS: Message in the mod list when a mod is enabled with error (danger/sale #e34848)
					hard_deps[i] = minetest.colorize("#e34848", dep_name .. " " .. fgettext("(Enabled, has error)"))
				else
					-- Success/green #3cb371
					hard_deps[i] = minetest.colorize("#3cb371", dep_name)
				end
			end
			for i, dep_name in ipairs(soft_deps) do
				local dep = enabled_mods_by_name[dep_name]
				if dep and with_error[dep.virtual_path] then
					soft_deps[i] = minetest.colorize("#e34848", dep_name .. " " .. fgettext("(Enabled, has error)"))
				elseif dep then
					soft_deps[i] = minetest.colorize("#3cb371", dep_name)
				end
			end
		end

		local hard_deps_str = table.concat(hard_deps, ",")
		local soft_deps_str = table.concat(soft_deps, ",")

		retval = retval ..
			"style[mod_lbl;font=bold;font_size=+12;textcolor=#f2f2f4]" ..
			"label[0,0;mod_lbl;" .. fgettext("Mod:") .. "]" ..
			"style[mod_val;font=bold;font_size=+12;textcolor=#3a7bfd]" ..
			"label[1.0,0;mod_val;" .. mod.name .. "]"

		if hard_deps_str == "" then
			if soft_deps_str == "" then
				retval = retval ..
					"style[no_deps;textcolor=#8b8b92]" ..
					"label[0,0.85;no_deps;" ..
					-- TRANSLATORS: About mod dependencies
					fgettext("No (optional) dependencies") .. "]"
			else
				retval = retval ..
					-- TRANSLATORS: About mod dependencies
					"style[dep_lbl;textcolor=#f2f2f4]" ..
					"label[0,0.85;dep_lbl;" .. fgettext("No hard dependencies") ..
					"]" ..
					-- TRANSLATORS: About mod dependencies
					"label[0,1.35;dep_lbl;" .. fgettext("Optional dependencies:") ..
					"]" ..
					"style[world_config_optdepends;bgcolor=#0d0d0f;textcolor=#f2f2f4;border=true;border_color=#232326;highlight_bgcolor=#16161a]" ..
					"textlist[0,1.85;5.2,4.6;world_config_optdepends;" ..
					soft_deps_str .. ";0]"
			end
		else
			if soft_deps_str == "" then
				retval = retval ..
					-- TRANSLATORS: About mod dependencies
					"style[dep_lbl;textcolor=#f2f2f4]" ..
					"label[0,0.85;dep_lbl;" .. fgettext("Dependencies:") .. "]" ..
					"style[world_config_depends;bgcolor=#0d0d0f;textcolor=#f2f2f4;border=true;border_color=#232326;highlight_bgcolor=#16161a]" ..
					"textlist[0,1.35;5.2,5.1;world_config_depends;" ..
					hard_deps_str .. ";0]" ..
					-- TRANSLATORS: About mod dependencies
					"style[no_opt_lbl;textcolor=#8b8b92]" ..
					"label[0,6.6;no_opt_lbl;" .. fgettext("No optional dependencies") .. "]"
			else
				retval = retval ..
					-- TRANSLATORS: About mod dependencies
					"style[dep_lbl;textcolor=#f2f2f4]" ..
					"label[0,0.85;dep_lbl;" .. fgettext("Dependencies:") .. "]" ..
					"style[world_config_depends,world_config_optdepends;bgcolor=#0d0d0f;textcolor=#f2f2f4;border=true;border_color=#232326;highlight_bgcolor=#16161a]" ..
					"textlist[0,1.35;5.2,2.3;world_config_depends;" ..
					hard_deps_str .. ";0]" ..
					-- TRANSLATORS: About mod dependencies
					"label[0,3.8;dep_lbl;" .. fgettext("Optional dependencies:") ..
					"]" ..
					"textlist[0,4.3;5.2,2.3;world_config_optdepends;" ..
					soft_deps_str .. ";0]"
			end
		end
	end

	retval = retval ..
		"container_end[]" .. -- Left Pane container end

		-- Right Pane Card (Layer 2) - flat fill #0d0d0f with border #232326
		"box[6.0,0.8;5.8,7.2;#0d0d0f]" ..
		"box[6.0,0.8;5.8,7.2;#232326;true]" ..
		"container[6.2,1.0]"

	-- Re-skin toggle buttons to dark marketplace specifications
	if mod.name ~= "" and not mod.always_on then
		if mod.is_modpack then
			if pkgmgr.is_modpack_entirely_enabled(data.list:get_raw_list(), mod) then
				formspec_tmp = "style[btn_mp_disable;bgcolor=#0d0d0f;textcolor=#e34848;border=true;border_color=#232326]" ..
					"style[btn_mp_disable:hovered;bgcolor=#16161a]" ..
					"button[0,0;2.5,0.6;btn_mp_disable;" .. fgettext("Disable modpack") .. "]"
				retval = retval .. formspec_tmp
			else
				formspec_tmp = "style[btn_mp_enable;bgcolor=#3a7bfd;textcolor=#ffffff;border=false]" ..
					"style[btn_mp_enable:hovered;bgcolor=#2f68d8]" ..
					"button[0,0;2.5,0.6;btn_mp_enable;" .. fgettext("Enable modpack") .. "]"
				retval = retval .. formspec_tmp
			end
		else
			retval = retval ..
				"style[cb_mod_enable;textcolor=#8b8b92]" ..
				"checkbox[0,-0.1;cb_mod_enable;" .. fgettext("enabled") ..
				";" .. tostring(mod.enabled) .. "]"
		end
	end
	if enabled_all then
		retval = retval ..
			"style[btn_disable_all_mods;bgcolor=#0d0d0f;textcolor=#8b8b92;border=true;border_color=#232326]" ..
			"style[btn_disable_all_mods:hovered;bgcolor=#16161a]" ..
			"button[2.8,0;2.6,0.6;btn_disable_all_mods;" .. fgettext("Disable all") .. "]"
	else
		retval = retval ..
			"style[btn_enable_all_mods;bgcolor=#0d0d0f;textcolor=#8b8b92;border=true;border_color=#232326]" ..
			"style[btn_enable_all_mods:hovered;bgcolor=#16161a]" ..
			"button[2.8,0;2.6,0.6;btn_enable_all_mods;" .. fgettext("Enable all") .. "]"
	end

	local use_technical_names = core.settings:get_bool("show_technical_names")

	retval = retval ..
		"style[world_config_modlist;bgcolor=#0d0d0f;textcolor=#f2f2f4;border=true;border_color=#232326;highlight_bgcolor=#16161a;highlight_textcolor=#f2f2f4]" ..
		"tablecolumns[color;tree;image,align=inline,width=1.5,0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") ..
			",1=" .. core.formspec_escape(defaulttexturedir .. "checkbox_16.png") ..
			",2=" .. core.formspec_escape(defaulttexturedir .. "error_icon_orange.png") ..
			",3=" .. core.formspec_escape(defaulttexturedir .. "error_icon_red.png") .. ";text]" ..
		"table[0,0.8;5.4,6.0;world_config_modlist;" ..
		pkgmgr.render_packagelist(data.list, use_technical_names, with_error) .. ";" .. data.selected_mod .."]" ..
		"container_end[]" -- Right Pane container end

	-- Action buttons at bottom strip
	retval = retval ..
		"style[btn_config_world_save;bgcolor=#3a7bfd;textcolor=#ffffff;font=bold;border=false]" ..
		"style[btn_config_world_save:hovered;bgcolor=#2f68d8]" ..
		"button[0.2,8.2;3.7,0.6;btn_config_world_save;" .. fgettext("Save") .. "]" ..

		"style[btn_config_world_cancel;bgcolor=#0d0d0f;textcolor=#8b8b92;border=true;border_color=#232326]" ..
		"style[btn_config_world_cancel:hovered;bgcolor=#16161a;textcolor=#f2f2f4]" ..
		"button[4.15,8.2;3.7,0.6;btn_config_world_cancel;" .. fgettext("Cancel") .. "]" ..

		"style[btn_config_world_cdb;bgcolor=#0d0d0f;textcolor=#8b8b92;border=true;border_color=#232326]" ..
		"style[btn_config_world_cdb:hovered;bgcolor=#16161a;textcolor=#f2f2f4]" ..
		"button[8.1,8.2;3.7,0.6;btn_config_world_cdb;" .. fgettext("Find More Mods") .. "]"

	return retval
end

local function handle_buttons(this, fields)
	if fields.world_config_modlist then
		local event = core.explode_table_event(fields.world_config_modlist)
		this.data.selected_mod = event.row
		core.settings:set("world_config_selected_mod", event.row)

		if event.type == "DCL" then
			pkgmgr.enable_mod(this)
		end

		return true
	end

	if fields.key_enter then
		pkgmgr.enable_mod(this)
		return true
	end

	if fields.cb_mod_enable ~= nil then
		pkgmgr.enable_mod(this, core.is_yes(fields.cb_mod_enable))
		return true
	end

	if fields.btn_mp_enable ~= nil or
			fields.btn_mp_disable then
		pkgmgr.enable_mod(this, fields.btn_mp_enable ~= nil)
		return true
	end

	if fields.btn_config_world_save then
		local filename = this.data.worldspec.path .. DIR_DELIM .. "world.mt"

		local worldfile = Settings(filename)
		local mods = worldfile:to_table()

		local rawlist = this.data.list:get_raw_list()
		local was_set = {}

		for i = 1, #rawlist do
			local mod = rawlist[i]
			if not mod.is_modpack and not mod.always_on then
				if modname_valid(mod.name) then
					if mod.enabled then
						worldfile:set("load_mod_" .. mod.name, mod.virtual_path)
						was_set[mod.name] = true
					elseif not was_set[mod.name] then
						worldfile:remove("load_mod_" .. mod.name)
					end
				elseif mod.enabled then
					gamedata.errormessage = fgettext_ne("Failed to enable mo" ..
							"d \"$1\" as it contains disallowed characters. " ..
							"Only characters [a-z0-9_] are allowed.",
							mod.name)
				end
				mods["load_mod_" .. mod.name] = nil
			end
		end

		-- Remove mods that are not present anymore
		for key in pairs(mods) do
			if key:sub(1, 9) == "load_mod_" then
				worldfile:remove(key)
			end
		end

		if not worldfile:write() then
			core.log("error", "Failed to write world config file")
		end

		this:delete()
		return true
	end

	if fields.btn_config_world_cancel then
		this:delete()
		return true
	end

	if fields.btn_config_world_cdb then
		this.data.list = nil

		local dlg = create_contentdb_dlg("mod")
		dlg:set_parent(this)
		this:hide()
		dlg:show()
		return true
	end

	if fields.btn_enable_all_mods then
		local list = this.data.list:get_raw_list()

		-- When multiple copies of a mod are installed, we need to avoid enabling multiple of them
		-- at a time. So lets first collect all the enabled mods, and then use this to exclude
		-- multiple enables.

		local was_enabled = {}
		for _, mod in ipairs(list) do
			if not mod.always_on and not mod.is_modpack and mod.enabled then
				was_enabled[mod.name] = true
			end
		end

		for _, mod in ipairs(list) do
			if not mod.always_on and not mod.is_modpack and not was_enabled[mod.name] then
				mod.enabled = true
			end
		end

		enabled_all = true
		return true
	end

	if fields.btn_disable_all_mods then
		local list = this.data.list:get_raw_list()

		for _, mod in ipairs(list) do
			if not mod.always_on and not mod.is_modpack then
				mod.enabled = false
			end
		end
		enabled_all = false
		return true
	end

	return false
end

function create_configure_world_dlg(worldidx)
	local dlg = dialog_create("sp_config_world", get_formspec, handle_buttons)

	dlg.data.selected_mod = tonumber(
			core.settings:get("world_config_selected_mod")) or 0

	dlg.data.worldspec = core.get_worlds()[worldidx]
	if not dlg.data.worldspec then
		dlg:delete()
		return
	end

	dlg.data.worldconfig = pkgmgr.get_worldconfig(dlg.data.worldspec.path)

	if not dlg.data.worldconfig or not dlg.data.worldconfig.id or
			dlg.data.worldconfig.id == "" then
		dlg:delete()
		return
	end

	return dlg
end
