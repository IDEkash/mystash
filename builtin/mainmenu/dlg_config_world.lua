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

	-- Premium Gradient Layer 1 box & Larger Layout Setup
	local retval =
		"formspec_version[7]" ..
		"size[12.5,8.8]" ..
		"padding[0.2,0.2]" ..
		"bgcolor[;true]" ..
		"style_type[box;colors=#0f172a,#1e293b,#1e293b,#0f172a]" ..
		"box[-0.5,-0.5;13.5,9.8;]" .. -- Solid Layer 1

		-- Title Header Area
		"style[title_lbl;font=bold;font_size=+12;textcolor=white]" ..
		"label[0.3,0.3;title_lbl;" .. fgettext("Configure Mods for:") .. "]" ..
		"style[world_lbl;font=bold;font_size=+12;textcolor=#38bdf8]" ..
		"label[4.2,0.3;world_lbl;" .. core.formspec_escape(data.worldspec.name) .. "]" ..

		-- Left Pane Card (Layer 2)
		"box[0.2,0.8;5.8,7.0;#1e293bB0]" ..
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
			"textarea[0,0;5.4,5.6;;" .. info .. ";]"
	elseif mod.type == "worldmods" then
		retval = retval ..
			"textarea[0,0;5.4,5.6;;" ..
			fgettext("Mods located inside the world folder.") .. ";]"
	else
		local hard_deps, soft_deps = pkgmgr.get_dependencies(mod.path)

		-- Add error messages to dep lists
		if mod.enabled or mod.always_on then
			for i, dep_name in ipairs(hard_deps) do
				local dep = enabled_mods_by_name[dep_name]
				if not dep then
					-- TRANSLATORS: Displayed when a mod dependency is unsatisfied
					hard_deps[i] = mt_color_red .. dep_name .. " " .. fgettext("(Unsatisfied)")
				elseif with_error[dep.virtual_path] then
					-- TRANSLATORS: Message in the mod list when a mod is enabled with error
					hard_deps[i] = mt_color_orange .. dep_name .. " " .. fgettext("(Enabled, has error)")
				else
					hard_deps[i] = mt_color_green .. dep_name
				end
			end
			for i, dep_name in ipairs(soft_deps) do
				local dep = enabled_mods_by_name[dep_name]
				if dep and with_error[dep.virtual_path] then
					soft_deps[i] = mt_color_orange .. dep_name .. " " .. fgettext("(Enabled, has error)")
				elseif dep then
					soft_deps[i] = mt_color_green .. dep_name
				end
			end
		end

		local hard_deps_str = table.concat(hard_deps, ",")
		local soft_deps_str = table.concat(soft_deps, ",")

		-- Place selected mod's name below the label on its own line to prevent overflows and overlaps
		retval = retval ..
			"style[mod_lbl;font=bold;font_size=+11;textcolor=white]" ..
			"label[0,0;mod_lbl;" .. fgettext("Mod:") .. "]" ..
			"style[mod_val;font=bold;font_size=+11;textcolor=#38bdf8]" ..
			"label[0,0.4;mod_val;" .. core.formspec_escape(mod.name) .. "]"

		if hard_deps_str == "" then
			if soft_deps_str == "" then
				retval = retval ..
					"label[0,1.2;" ..
					-- TRANSLATORS: About mod dependencies
					fgettext("No (optional) dependencies") .. "]"
			else
				retval = retval ..
					-- TRANSLATORS: About mod dependencies
					"label[0,1.2;" .. fgettext("No hard dependencies") ..
					"]" ..
					-- TRANSLATORS: About mod dependencies
					"label[0,1.7;" .. fgettext("Optional dependencies:") ..
					"]" ..
					"textlist[0,2.2;5.4,3.4;world_config_optdepends;" ..
					soft_deps_str .. ";0]"
			end
		else
			if soft_deps_str == "" then
				retval = retval ..
					-- TRANSLATORS: About mod dependencies
					"label[0,1.2;" .. fgettext("Dependencies:") .. "]" ..
					"textlist[0,1.7;5.4,3.9;world_config_depends;" ..
					hard_deps_str .. ";0]" ..
					-- TRANSLATORS: About mod dependencies
					"label[0,5.8;" .. fgettext("No optional dependencies") .. "]"
			else
				retval = retval ..
					-- TRANSLATORS: About mod dependencies
					"label[0,1.2;" .. fgettext("Dependencies:") .. "]" ..
					"textlist[0,1.7;5.4,2.0;world_config_depends;" ..
					hard_deps_str .. ";0]" ..
					-- TRANSLATORS: About mod dependencies
					"label[0,3.9;" .. fgettext("Optional dependencies:") ..
					"]" ..
					"textlist[0,4.4;5.4,1.4;world_config_optdepends;" ..
					soft_deps_str .. ";0]"
			end
		end
	end

	retval = retval ..
		"container_end[]" .. -- Left Pane container end

		-- Right Pane Card (Layer 2)
		"box[6.2,0.8;6.1,7.0;#1e293bB0]" ..
		"container[6.4,1.0]"

	if mod.name ~= "" and not mod.always_on then
		if mod.is_modpack then
			if pkgmgr.is_modpack_entirely_enabled(data.list:get_raw_list(), mod) then
				retval = retval ..
					"style[btn_mp_disable;bgcolor=#9b2c2c;textcolor=white;border=false]" ..
					"style[btn_mp_disable:hovered;bgcolor=#b91c1c]" ..
					"style[btn_mp_disable:pressed;bgcolor=#7f1d1d]" ..
					"button[0,0;2.6,0.6;btn_mp_disable;" .. fgettext("Disable modpack") .. "]"
			else
				retval = retval ..
					"style[btn_mp_enable;bgcolor=#0284c7;textcolor=white;border=false]" ..
					"style[btn_mp_enable:hovered;bgcolor=#0369a1]" ..
					"style[btn_mp_enable:pressed;bgcolor=#075985]" ..
					"button[0,0;2.6,0.6;btn_mp_enable;" .. fgettext("Enable modpack") .. "]"
			end
		else
			retval = retval ..
				"checkbox[0,-0.125;cb_mod_enable;" .. fgettext("enabled") ..
				";" .. tostring(mod.enabled) .. "]"
		end
	end
	if enabled_all then
		retval = retval ..
			"style[btn_disable_all_mods;bgcolor=#475569;textcolor=white;border=false]" ..
			"style[btn_disable_all_mods:hovered;bgcolor=#64748b]" ..
			"style[btn_disable_all_mods:pressed;bgcolor=#334155]" ..
			"button[3.1,0;2.6,0.6;btn_disable_all_mods;" .. fgettext("Disable all") .. "]"
	else
		retval = retval ..
			"style[btn_enable_all_mods;bgcolor=#334155;textcolor=white;border=false]" ..
			"style[btn_enable_all_mods:hovered;bgcolor=#475569]" ..
			"style[btn_enable_all_mods:pressed;bgcolor=#1e293b]" ..
			"button[3.1,0;2.6,0.6;btn_enable_all_mods;" .. fgettext("Enable all") .. "]"
	end

	local use_technical_names = core.settings:get_bool("show_technical_names")

	retval = retval ..
		"tablecolumns[color;tree;image,align=inline,width=1.5,0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") ..
			",1=" .. core.formspec_escape(defaulttexturedir .. "checkbox_16.png") ..
			",2=" .. core.formspec_escape(defaulttexturedir .. "error_icon_orange.png") ..
			",3=" .. core.formspec_escape(defaulttexturedir .. "error_icon_red.png") .. ";text]" ..
		"table[0,0.8;5.7,5.8;world_config_modlist;" ..
		pkgmgr.render_packagelist(data.list, use_technical_names, with_error) .. ";" .. data.selected_mod .."]" ..
		"container_end[]" -- Right Pane container end

	-- Action buttons at bottom strip
	retval = retval ..
		"style[btn_config_world_save;bgcolor=#0284c7;textcolor=white;font=bold;border=false]" ..
		"style[btn_config_world_save:hovered;bgcolor=#0369a1]" ..
		"style[btn_config_world_save:pressed;bgcolor=#075985]" ..
		"button[0.2,8.0;3.8,0.6;btn_config_world_save;" .. fgettext("Save") .. "]" ..

		"style[btn_config_world_cancel;bgcolor=#9b2c2c;textcolor=white;border=false]" ..
		"style[btn_config_world_cancel:hovered;bgcolor=#b91c1c]" ..
		"style[btn_config_world_cancel:pressed;bgcolor=#7f1d1d]" ..
		"button[4.35,8.0;3.8,0.6;btn_config_world_cancel;" .. fgettext("Cancel") .. "]" ..

		"style[btn_config_world_cdb;bgcolor=#334155;textcolor=white;border=false]" ..
		"style[btn_config_world_cdb:hovered;bgcolor=#475569]" ..
		"style[btn_config_world_cdb:pressed;bgcolor=#1e293b]" ..
		"button[8.5,8.0;3.8,0.6;btn_config_world_cdb;" .. fgettext("Find More Mods") .. "]"

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
