-- Luanti
-- Copyright (C) 2014 sapier
-- Copyright (C) 2018 rubenwardy <rw@rubenwardy.com>
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function get_content_icons(packages_with_updates)
	local ret = {}
	for _, content in ipairs(packages_with_updates) do
		ret[content.virtual_path or content.path] = { type = "update" }
	end
	return ret
end


local packages_raw, packages

local function update_packages()
	pkgmgr.load_all()

	packages_raw = {}
	table.insert_all(packages_raw, pkgmgr.games)
	table.insert_all(packages_raw, pkgmgr.texture_packs)
	table.insert_all(packages_raw, pkgmgr.global_mods:get_list())

	local function get_data()
		return packages_raw
	end

	local function is_equal(element, uid) --uid match
		return (element.type == "game" and element.id == uid) or
				element.name == uid
	end

	packages = filterlist.create(get_data, pkgmgr.compare_package,
			is_equal, nil, {})
end

local function on_change(type)
	if type == "ENTER" then
		mm_game_theme.set_engine()
		update_packages()
	end
end

local function get_formspec(tabview, name, tabdata)
	if not packages then
		update_packages()
	end

	if not tabdata.selected_pkg then
		tabdata.selected_pkg = 1
	end

	local MARGIN = 0.5
	local LIST_W = 6.5
	local DETAILS_X = MARGIN + LIST_W + 0.5
	local DETAILS_W = tabview.width - DETAILS_X - MARGIN

	local use_technical_names = core.settings:get_bool("show_technical_names")
	local packages_with_updates = update_detector.get_all()
	local update_icons = get_content_icons(packages_with_updates)
	local update_count = #packages_with_updates
	local contentdb_label = update_count == 0 and fgettext("Browse online content") or fgettext("Browse online content [$1]", update_count)

	local retval = {
		"container[" .. MARGIN .. "," .. MARGIN .. "]",
		"label[0,0.1;", fgettext("Installed Packages:"), "]",
		"tablecolumns[color;tree;image,align=inline,width=1.5",
			",tooltip=", fgettext("Update available?"),
			",0=", core.formspec_escape(defaulttexturedir .. "blank.png"),
			",4=", core.formspec_escape(defaulttexturedir .. "cdb_update_cropped.png"),
			";text]",
		"table[0,0.5;" .. LIST_W .. ",5;pkglist;",
		pkgmgr.render_packagelist(packages, use_technical_names, update_icons),
		";", tabdata.selected_pkg, "]",

		"style[btn_contentdb;bgcolor=#467832;textcolor=white;font=bold]",
		"button[0,5.7;" .. LIST_W .. ",0.9;btn_contentdb;", contentdb_label, "]",
		"tooltip[btn_contentdb;" .. fgettext("Download mods, games, and texture packs from ContentDB") .. "]",
		"container_end[]"
	}

	local selected_pkg
	if filterlist.size(packages) >= tabdata.selected_pkg then
		selected_pkg = packages:get_list()[tabdata.selected_pkg]
	end

	if selected_pkg then
		local valid_screenshots = {
			selected_pkg.path .. DIR_DELIM .. "screenshot.png",
			selected_pkg.path .. DIR_DELIM .. "screenshot.jpg",
			selected_pkg.path .. DIR_DELIM .. "screenshot.jpeg",
		}

		local modscreenshot
		for _, screenshotfilename in ipairs(valid_screenshots) do
			local screenshotfile, err = io.open(screenshotfilename, "r")
			if not err then
				screenshotfile:close()
				modscreenshot = screenshotfilename
				break
			end
		end
		modscreenshot = modscreenshot or (defaulttexturedir .. "no_screenshot.png")

		local desc = (selected_pkg.description and selected_pkg.description:trim() ~= "") and core.formspec_escape(selected_pkg.description) or fgettext("No package description available")
		local info = core.get_content_info(selected_pkg.path)

		local title_and_name = selected_pkg.type == "game" and (selected_pkg.title or selected_pkg.name) or
			((selected_pkg.title or selected_pkg.name) .. "\n" .. core.colorize("#BFBFBF", selected_pkg.name))

		table.insert_all(retval, {
			"container[" .. DETAILS_X .. "," .. MARGIN .. "]",
			"image[0,0;3,2;" .. core.formspec_escape(modscreenshot) .. "]",
			"label[3.2,0.5;" .. core.formspec_escape(title_and_name) .. "]",
			"box[0,2.2;" .. DETAILS_W .. ",3.4;#00000080]",
			"textarea[0.1,2.3;" .. (DETAILS_W-0.2) .. ",3.2;;;" .. desc .. "]"
		})

		local btn_y = 5.7
		local btn_w = (DETAILS_W - 0.1) / 2

		if selected_pkg.is_modpack then
			table.insert_all(retval, {
				"style[btn_mod_mgr_rename_modpack;bgcolor=#43464b;textcolor=white]",
				"button[0,2.2;" .. DETAILS_W .. ",0.5;btn_mod_mgr_rename_modpack;" .. fgettext("Rename") .. "]",
				"tooltip[btn_mod_mgr_rename_modpack;" .. fgettext("Rename this modpack") .. "]"
			})
		elseif selected_pkg.type == "txp" then
			local txp_label = selected_pkg.enabled and fgettext("Disable Texture Pack") or fgettext("Use Texture Pack")
			local txp_style = selected_pkg.enabled and "bgcolor=#43464b;textcolor=white" or "bgcolor=#467832;textcolor=white"
			table.insert_all(retval, {
				"style[btn_mod_mgr_txp;" .. txp_style .. "]",
				"button[0,2.2;" .. DETAILS_W .. ",0.5;" .. (selected_pkg.enabled and "btn_mod_mgr_disable_txp" or "btn_mod_mgr_use_txp") .. ";" .. txp_label .. "]"
			})
		end

		if core.may_modify_path(selected_pkg.path) then
			table.insert_all(retval, {
				"style[btn_mod_mgr_delete_mod;bgcolor=red;textcolor=white]",
				"button[0," .. btn_y .. ";" .. btn_w .. ",0.9;btn_mod_mgr_delete_mod;" .. fgettext("Uninstall") .. "]",
				"tooltip[btn_mod_mgr_delete_mod;" .. fgettext("Uninstall this package") .. "]"
			})
		end

		if update_icons[selected_pkg.virtual_path or selected_pkg.path] then
			table.insert_all(retval, {
				"style[btn_mod_mgr_update;bgcolor=#467832;textcolor=white]",
				"button[" .. (DETAILS_W - btn_w) .. "," .. btn_y .. ";" .. btn_w .. ",0.9;btn_mod_mgr_update;" .. fgettext("Update") .. "]",
				"tooltip[btn_mod_mgr_update;" .. fgettext("Update this package") .. "]"
			})
		end

		table.insert(retval, "container_end[]")
	end

	return table.concat(retval)
end

local function handle_doubleclick(pkg)
	if pkg.type == "txp" then
		if core.settings:get("texture_path") == pkg.path then
			core.settings:set("texture_path", "")
		else
			core.settings:set("texture_path", pkg.path)
		end
		packages = nil
		pkgmgr.reload_texture_packs()

		mm_game_theme.init()
		mm_game_theme.set_engine()
	end
end

local function handle_buttons(tabview, fields, tabname, tabdata)

	if fields.pkglist then
		local event = core.explode_table_event(fields.pkglist)
		tabdata.selected_pkg = event.row
		if event.type == "DCL" then
			handle_doubleclick(packages:get_list()[tabdata.selected_pkg])
		end
		return true
	end

	if fields.btn_contentdb then
		local dlg = create_contentdb_dlg()
		dlg:set_parent(tabview)
		tabview:hide()
		dlg:show()
		packages = nil
		return true
	end

	if fields.btn_mod_mgr_rename_modpack then
		local mod = packages:get_list()[tabdata.selected_pkg]
		local dlg_renamemp = create_rename_modpack_dlg(mod)
		dlg_renamemp:set_parent(tabview)
		tabview:hide()
		dlg_renamemp:show()
		packages = nil
		return true
	end

	if fields.btn_mod_mgr_delete_mod then
		local mod = packages:get_list()[tabdata.selected_pkg]
		local dlg_delmod = create_delete_content_dlg(mod)
		dlg_delmod:set_parent(tabview)
		tabview:hide()
		dlg_delmod:show()
		packages = nil
		return true
	end

	if fields.btn_mod_mgr_update then
		local pkg = packages:get_list()[tabdata.selected_pkg]
		local dlg = create_contentdb_dlg(nil, pkgmgr.get_contentdb_id(pkg))
		dlg:set_parent(tabview)
		tabview:hide()
		dlg:show()
		packages = nil
		return true
	end

	if fields.btn_mod_mgr_use_txp or fields.btn_mod_mgr_disable_txp then
		local txp_path = ""
		if fields.btn_mod_mgr_use_txp then
			txp_path = packages:get_list()[tabdata.selected_pkg].path
		end

		core.settings:set("texture_path", txp_path)
		packages = nil
		pkgmgr.reload_texture_packs()

		mm_game_theme.init()
		mm_game_theme.set_engine()
		return true
	end

	return false
end

return {
	name = "content",
	caption = function()
		local update_count = core.settings:get_bool("contentdb_enable_updates_indicator") and update_detector.get_count() or 0
		if update_count == 0 then
			return fgettext("Content")
		else
			-- TRANSLATORS: $1 = number of available updates
			return fgettext("Content [$1]", update_count)
		end
	end,
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = on_change
}
