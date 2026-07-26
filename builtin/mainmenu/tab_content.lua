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

	local use_technical_names = core.settings:get_bool("show_technical_names")

	local packages_with_updates = update_detector.get_all()
	local update_icons = get_content_icons(packages_with_updates)
	local update_count = #packages_with_updates
	local contentdb_label
	if update_count == 0 then
		contentdb_label = fgettext("Browse online content")
	else
		-- TRANSLATORS: $1 = number of available updates
		contentdb_label = fgettext("Browse online content [$1]", update_count)
	end

	local retval = {
		"style[btn_contentdb;bgcolor=#0284c7;textcolor=white;font=bold]",
		"style[btn_contentdb:hovered;bgcolor=#0369a1]",
		"style[title_lbl;font=bold;font_size=+14;textcolor=#38bdf8]",
		"style[btn_mod_mgr_delete_mod;bgcolor=#9b2c2c;textcolor=white]",
		"style[btn_mod_mgr_delete_mod:hovered;bgcolor=#b91c1c]",
		"style[btn_mod_mgr_update;bgcolor=#0284c7;textcolor=white;font=bold]",
		"style[btn_mod_mgr_update:hovered;bgcolor=#0369a1]",
		"style[btn_mod_mgr_use_txp;bgcolor=#0ea5e9;textcolor=white]",
		"style[btn_mod_mgr_use_txp:hovered;bgcolor=#0284c7]",
		"style[btn_mod_mgr_disable_txp;bgcolor=#9b2c2c;textcolor=white]",
		"style[btn_mod_mgr_disable_txp:hovered;bgcolor=#b91c1c]",
		"style[btn_mod_mgr_rename_modpack;bgcolor=#334155;textcolor=white]",
		"style[btn_mod_mgr_rename_modpack:hovered;bgcolor=#475569]",

		"background9[0.15,0.15;6.6,6.8;button_hover_semitrans.png;false;6,6]",
		"background9[6.9,0.15;8.45,6.8;button_hover_semitrans.png;false;6,6]",

		"label[0.35,0.35;", fgettext("Installed Packages:"), "]",
		"tablecolumns[color;tree;image,align=inline,width=1.5",
			",tooltip=", fgettext("Update available?"),
			",0=", core.formspec_escape(defaulttexturedir .. "blank.png"),
			",4=", core.formspec_escape(defaulttexturedir .. "cdb_update_cropped.png"),
			";text]",
		"table[0.35,0.75;6.2,4.85;pkglist;",
		pkgmgr.render_packagelist(packages, use_technical_names, update_icons),
		";", tabdata.selected_pkg, "]",

		"button[0.35,5.8;6.2,0.8;btn_contentdb;", contentdb_label, "]"
	}

	local selected_pkg
	if filterlist.size(packages) >= tabdata.selected_pkg then
		selected_pkg = packages:get_list()[tabdata.selected_pkg]
	end

	if selected_pkg then
		local valid_screenshots = {
			-- See also contentdb/app/tasks/importtasks.py, def import_repo_screenshot
			selected_pkg.path .. DIR_DELIM .. "screenshot.png",
			selected_pkg.path .. DIR_DELIM .. "screenshot.jpg",
			selected_pkg.path .. DIR_DELIM .. "screenshot.jpeg",
		}

		-- Check for screenshot being available
		local modscreenshot
		for _, screenshotfilename in ipairs(valid_screenshots) do
			local screenshotfile, err = io.open(screenshotfilename, "r")
			if not err then
				screenshotfile:close()
				modscreenshot = screenshotfilename
				break
			end
		end

		-- Fallback to no_screenshot if no screenshot is avaliable
		if not modscreenshot then
			modscreenshot = defaulttexturedir .. "no_screenshot.png"
		end

		local desc = fgettext("No package description available")
		if selected_pkg.description and selected_pkg.description:trim() ~= "" then
			desc = core.formspec_escape(selected_pkg.description)
		end

		local info = core.get_content_info(selected_pkg.path)

		local title_and_name
		if selected_pkg.type == "game" then
			title_and_name = selected_pkg.title or selected_pkg.name
		else
			title_and_name = (selected_pkg.title or selected_pkg.name) .. "\n" ..
				core.colorize("#94a3b8", selected_pkg.name)
		end

		local desc_height = 3.2

		if selected_pkg.is_modpack then
			desc_height = 2.1

			table.insert_all(retval, {
				"button[7.1,4.8;8.05,0.8;btn_mod_mgr_rename_modpack;",
				fgettext("Rename"), "]"
			})
		elseif selected_pkg.type == "mod" then
			-- Show dependencies for mods
			desc = desc .. "\n\n"
			local toadd_hard = table.concat(info.depends or {}, "\n")
			local toadd_soft = table.concat(info.optional_depends or {}, "\n")
			if toadd_hard == "" and toadd_soft == "" then
				desc = desc .. fgettext("No dependencies.")
			else
				if toadd_hard ~= "" then
					desc = desc ..fgettext("Dependencies:") ..
						"\n" .. toadd_hard
				end
				if toadd_soft ~= "" then
					if toadd_hard ~= "" then
						desc = desc .. "\n\n"
					end
					desc = desc .. fgettext("Optional dependencies:") ..
						"\n" .. toadd_soft
				end
			end
		elseif selected_pkg.type == "txp" then
			desc_height = 2.1

			if selected_pkg.enabled then
				table.insert_all(retval, {
					"button[7.1,4.8;8.05,0.8;btn_mod_mgr_disable_txp;",
					fgettext("Disable Texture Pack"), "]"
				})
			else
				table.insert_all(retval, {
					"button[7.1,4.8;8.05,0.8;btn_mod_mgr_use_txp;",
					fgettext("Use Texture Pack"), "]"
				})
			end
		end

		table.insert_all(retval, {
			"image[7.1,0.35;3,2;", core.formspec_escape(modscreenshot), "]",
			"label[10.3,0.35;title_lbl;", core.formspec_escape(title_and_name), "]",
			"background9[7.1,2.5;8.05,", tostring(desc_height), ";button_hover_semitrans.png;false;6,6]",
			"textarea[7.2,2.55;7.85,", tostring(desc_height - 0.1), ";;;", desc, "]",
		})

		if core.may_modify_path(selected_pkg.path) then
			table.insert_all(retval, {
				"button[7.1,5.8;3.9,0.8;btn_mod_mgr_delete_mod;",
				fgettext("Uninstall"), "]"
			})
		end

		if update_icons[selected_pkg.virtual_path or selected_pkg.path] then
			table.insert_all(retval, {
				"button[11.2,5.8;3.9,0.8;btn_mod_mgr_update;",
				fgettext("Update"), "]"
			})
		end
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
