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
	local MARGIN = 0.5
	local LEFT_W = 5.5
	local RIGHT_X = MARGIN + LEFT_W + 0.5
	local RIGHT_W = 12.5 - RIGHT_X - MARGIN

	local retval =
		"formspec_version[10]" ..
		"size[12.5,8.5]" ..
		"container[" .. MARGIN .. "," .. MARGIN .. "]" ..
		"label[0,0.1;" .. fgettext("Configure World:") .. "]" ..
		"label[2.5,0.1;" .. core.colorize("#fff", core.formspec_escape(data.worldspec.name)) .. "]"

	if mod.is_modpack or mod.type == "game" then
		local info = core.formspec_escape(core.get_content_info(mod.path).description)
		if info == "" then
			info = mod.is_modpack and fgettext("No modpack description provided.") or fgettext("No game description provided.")
		end
		retval = retval .. "box[0,0.6;" .. LEFT_W .. ",6.4;#00000080]" ..
				"textarea[0.1,0.7;" .. (LEFT_W-0.2) .. ",6.2;;;" .. info .. "]"
	elseif mod.type == "worldmods" then
		retval = retval .. "box[0,0.6;" .. LEFT_W .. ",6.4;#00000080]" ..
				"textarea[0.1,0.7;" .. (LEFT_W-0.2) .. ",6.2;;;" .. fgettext("Mods located inside the world folder.") .. "]"
	else
		local hard_deps, soft_deps = pkgmgr.get_dependencies(mod.path)
		if mod.enabled or mod.always_on then
			for i, dep_name in ipairs(hard_deps) do
				local dep = enabled_mods_by_name[dep_name]
				hard_deps[i] = not dep and (mt_color_red .. dep_name .. " " .. fgettext("(Unsatisfied)")) or
						(with_error[dep.virtual_path] and (mt_color_orange .. dep_name .. " " .. fgettext("(Error)")) or (mt_color_green .. dep_name))
			end
			for i, dep_name in ipairs(soft_deps) do
				local dep = enabled_mods_by_name[dep_name]
				if dep then
					soft_deps[i] = with_error[dep.virtual_path] and (mt_color_orange .. dep_name .. " " .. fgettext("(Error)")) or (mt_color_green .. dep_name)
				end
			end
		end

		local hard_deps_str = table.concat(hard_deps, ",")
		local soft_deps_str = table.concat(soft_deps, ",")

		retval = retval .. "label[0,0.7;" .. fgettext("Mod:") .. "]" ..
				"label[0.75,0.7;" .. core.colorize("#fff", mod.name) .. "]"

		if hard_deps_str == "" and soft_deps_str == "" then
			retval = retval .. "label[0,1.4;" .. fgettext("No dependencies") .. "]"
		else
			if hard_deps_str ~= "" then
				retval = retval .. "label[0,1.4;" .. fgettext("Dependencies:") .. "]" ..
						"textlist[0,1.9;" .. LEFT_W .. ",2;world_config_depends;" .. hard_deps_str .. ";0]"
			end
			if soft_deps_str ~= "" then
				local sy = hard_deps_str ~= "" and 4.2 or 1.4
				retval = retval .. "label[0," .. sy .. ";" .. fgettext("Optional dependencies:") .. "]" ..
						"textlist[0," .. (sy+0.5) .. ";" .. LEFT_W .. ",2.5;world_config_optdepends;" .. soft_deps_str .. ";0]"
			end
		end
	end
	retval = retval .. "container_end[]"

	-- Right Column: Mod List
	retval = retval .. "container[" .. RIGHT_X .. "," .. MARGIN .. "]" ..
		"tablecolumns[color;tree;image,align=inline,width=1.5,0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") ..
			",1=" .. core.formspec_escape(defaulttexturedir .. "checkbox_16.png") ..
			",2=" .. core.formspec_escape(defaulttexturedir .. "error_icon_orange.png") ..
			",3=" .. core.formspec_escape(defaulttexturedir .. "error_icon_red.png") .. ";text]" ..
		"table[0,0.6;" .. RIGHT_W .. ",5.8;world_config_modlist;" ..
		pkgmgr.render_packagelist(data.list, core.settings:get_bool("show_technical_names"), with_error) .. ";" .. data.selected_mod .."]"

	-- Mod Actions
	if mod.name ~= "" and not mod.always_on then
		if mod.is_modpack then
			local is_enabled = pkgmgr.is_modpack_entirely_enabled(data.list:get_raw_list(), mod)
			local btn_label = is_enabled and fgettext("Disable modpack") or fgettext("Enable modpack")
			retval = retval .. "style[btn_mp_toggle;bgcolor=#43464b;textcolor=white]" ..
				"button[0,0;" .. (RIGHT_W/2-0.1) .. ",0.5;" .. (is_enabled and "btn_mp_disable" or "btn_mp_enable") .. ";" .. btn_label .. "]"
		else
			retval = retval .. "checkbox[0,0.1;cb_mod_enable;" .. fgettext("Enabled") .. ";" .. tostring(mod.enabled) .. "]"
		end
	end

	local btn_all_label = enabled_all and fgettext("Disable all") or fgettext("Enable all")
	retval = retval .. "style[btn_all;bgcolor=#43464b;textcolor=white]" ..
		"button[" .. (RIGHT_W/2+0.1) .. ",0;" .. (RIGHT_W/2-0.1) .. ",0.5;" .. (enabled_all and "btn_disable_all_mods" or "btn_enable_all_mods") .. ";" .. btn_all_label .. "]"

	retval = retval .. "container_end[]"

	-- Bottom Actions
	retval = retval .. "container[" .. MARGIN .. ",7.5]" ..
		"style[btn_config_world_cancel;bgcolor=#43464b;textcolor=white]" ..
		"button[" .. (12.5 - MARGIN*2 - 6.2) .. ",0;3,0.8;btn_config_world_cancel;" .. fgettext("Cancel") .. "]" ..
		"style[btn_config_world_save;bgcolor=#467832;textcolor=white;font=bold]" ..
		"button[" .. (12.5 - MARGIN*2 - 3) .. ",0;3,0.8;btn_config_world_save;" .. fgettext("Save") .. "]" ..
		"button[0,0;3,0.8;btn_config_world_cdb;" .. fgettext("Find Mods") .. "]" ..
		"container_end[]"

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
