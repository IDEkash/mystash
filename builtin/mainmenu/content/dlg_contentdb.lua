-- Luanti
-- Copyright (C) 2018-20 rubenwardy
-- SPDX-License-Identifier: LGPL-2.1-or-later

if not core.get_http_api then
	function create_contentdb_dlg()
		return messagebox("contentdb",
				fgettext("ContentDB is not available when Luanti was compiled without cURL"))
	end
	return
end

-- Filter
local search_string = ""
local cur_page = 1
local filter_type

-- Automatic package installation
local auto_install_spec = nil


local filter_type_names = {
	{ "type_all", nil },
	{ "type_game", "game" },
	{ "type_mod", "mod" },
	{ "type_txp", "txp" },
}


-- Resolves the package specification stored in auto_install_spec into an actual package.
-- May only be called after the package list has been loaded successfully.
local function resolve_auto_install_spec()
	assert(contentdb.load_ok)

	if not auto_install_spec then
		return nil
	end

	local spec = contentdb.aliases[auto_install_spec] or auto_install_spec
	local resolved = nil

	for _, pkg in ipairs(contentdb.packages_full_unordered) do
		if pkg.id == spec then
			resolved = pkg
			break
		end
	end

	if not resolved then
		gamedata.errormessage = fgettext_ne("The package $1 was not found.", auto_install_spec)
		ui.update()

		auto_install_spec = nil
	end

	return resolved
end


-- Installs the package specified by auto_install_spec.
-- Only does something if:
-- a. The package list has been loaded successfully.
-- b. The ContentDB dialog is currently visible.
local function do_auto_install()
	if not contentdb.load_ok then
		return
	end

	local pkg = resolve_auto_install_spec()
	if not pkg then
		return
	end

	local contentdb_dlg = ui.find_by_name("contentdb")
	if not contentdb_dlg or contentdb_dlg.hidden then
		return
	end

	install_or_update_package(contentdb_dlg, pkg)
	auto_install_spec = nil
end


local function sort_and_filter_pkgs()
	contentdb.update_paths()
	contentdb.sort_packages()
	contentdb.filter_packages(search_string, filter_type)

	local auto_install_pkg = resolve_auto_install_spec()
	if auto_install_pkg then
		local idx = table.indexof(contentdb.packages, auto_install_pkg)
		if idx ~= -1 then
			table.remove(contentdb.packages, idx)
			table.insert(contentdb.packages, 1, auto_install_pkg)
		end
	end
end


local function load()
	if contentdb.load_ok then
		sort_and_filter_pkgs()
		return
	end
	if contentdb.loading then
		return
	end
	contentdb.fetch_pkgs(function(result)
		if result then
			sort_and_filter_pkgs()
			do_auto_install()
		end
		ui.update()
	end)
end


local function get_info_formspec(size, padding, text)
	return table.concat({
		"formspec_version[6]",
		"size[", size.x, ",", size.y, "]",
		"padding[0,0]",
		"bgcolor[;true]",

		"label[", padding.x + 3.625, ",4.35;", text, "]",
		"container[", padding.x, ",", size.y - 0.8 - padding.y, "]",
		"button[0,0;2,0.8;back;", fgettext("Back"), "]",
		"container_end[]",
	})
end


-- Determines how to fit `num_per_page` into `size` space
local function fit_cells(num_per_page, size)
	local cell_spacing = 0.5
	local columns = 1
	local cell_w, cell_h
	-- Fit cells into the available height
	while true do
		cell_w = (size.x - (columns-1)*cell_spacing) / columns
		cell_h = cell_w / 4

		local required_height = math.ceil(num_per_page / columns) * (cell_h + cell_spacing) - cell_spacing
		-- Add 0.1 to be more lenient
		if required_height <= size.y + 0.1 then
			break
		end

		columns = columns + 1
	end

	return cell_spacing, columns, cell_w, cell_h
end


local function calculate_num_per_page()
	local size = contentdb.get_formspec_size()
	local padding = contentdb.get_formspec_padding()
	local window = core.get_window_info()

	size.x = size.x - padding.x * 2
	size.y = size.y - padding.y * 2 - 1.425 - 0.25 - 0.8

	local coordToPx = window.size.x / window.max_formspec_size.x / window.real_gui_scaling

	local num_per_page = 12
	while num_per_page > 2 do
		local _, _, cell_w, _ = fit_cells(num_per_page, size)
		if cell_w * coordToPx > 350 then
			break
		end

		num_per_page = num_per_page - 1
	end
	return num_per_page
end


local function get_formspec(dlgdata)
	local window_padding = contentdb.get_formspec_padding()
	local size = contentdb.get_formspec_size()

	if contentdb.loading then
		return get_info_formspec(size, window_padding, fgettext("Loading..."))
	end
	if contentdb.load_error then
		return get_info_formspec(size, window_padding, fgettext("No packages could be retrieved"))
	end
	assert(contentdb.load_ok)

	contentdb.update_paths()

	local num_per_page = dlgdata.num_per_page
	dlgdata.pagemax = math.max(math.ceil(#contentdb.packages / num_per_page), 1)
	if cur_page > dlgdata.pagemax then
		cur_page = 1
	end

	local W = size.x - window_padding.x * 2
	local H = size.y - window_padding.y * 2

	local selected_type = filter_type

	local formspec = {
		"formspec_version[7]",
		"size[", size.x, ",", size.y, "]",
		"padding[0,0]",
		"bgcolor[;true]",

		-- Solid dark premium greyish blue backdrop (Layer 1)
		"box[-0.5,-0.5;", size.x + 1, ",", size.y + 1, ";#0f172aF2]",

		"container[", window_padding.x, ",", window_padding.y, "]",
	}

	-- Left: Horizontal category selector buttons (tabs behavior, premium greyish-blue style)
	local cat_buttons = {
		{ id = "type_all", label = fgettext("All"), active = (selected_type == nil), w = 1.4 },
		{ id = "type_game", label = fgettext("Games"), active = (selected_type == "game"), w = 1.8 },
		{ id = "type_mod", label = fgettext("Mods"), active = (selected_type == "mod"), w = 1.6 },
		{ id = "type_txp", label = fgettext("Textures"), active = (selected_type == "txp"), w = 2.4 },
	}

	local current_x = 0
	for _, cat in ipairs(cat_buttons) do
		local bg_col = cat.active and "#0284c7" or "#334155"
		local text_col = "white"
		local font_style = cat.active and "bold" or "normal"
		formspec[#formspec + 1] = ("style[%s;bgcolor=%s;textcolor=%s;border=false;font=%s]style[%s:hovered;bgcolor=#475569]button[%f,0;%f,0.8;%s;%s]"):format(
			cat.id, bg_col, text_col, font_style, cat.id, current_x, cat.w, cat.id, cat.label
		)
		if cat.active then
			formspec[#formspec + 1] = ("box[%f,0.74;%f,0.06;#38bdf8]"):format(current_x, cat.w)
		end
		current_x = current_x + cat.w + 0.15
	end

	-- Right: Search Box starting exactly at current_x
	local search_x = current_x
	local search_box_width = W - search_x - 1.6
	formspec[#formspec + 1] = ("style[search_string;bgcolor=#1e293b;textcolor=white;border=true;border_color=#334155]")
	formspec[#formspec + 1] = ("field[%f,0;%f,0.8;search_string;;%s]"):format(
		search_x, search_box_width, core.formspec_escape(search_string)
	)
	formspec[#formspec + 1] = "field_enter_after_edit[search_string;true]"
	formspec[#formspec + 1] = ("style[search;bgcolor=#334155;border=false]")
	formspec[#formspec + 1] = ("image_button[%f,0;0.8,0.8;%s;search;]"):format(
		search_x + search_box_width, core.formspec_escape(defaulttexturedir .. "search.png")
	)
	formspec[#formspec + 1] = ("style[clear;bgcolor=#334155;border=false]")
	formspec[#formspec + 1] = ("image_button[%f,0;0.8,0.8;%s;clear;]"):format(
		search_x + search_box_width + 0.8, core.formspec_escape(defaulttexturedir .. "clear.png")
	)

	-- Bottom strip start
	table.insert_all(formspec, {
		"container[0,", H - 0.8, "]",
		"style[back;bgcolor=#9b2c2c;textcolor=white]",
		"style[back:hovered;bgcolor=#b91c1c]",
		"button[0,0;2,0.8;back;", fgettext("Back"), "]",

		-- Bottom-center: Page nav buttons
		"container[", (W - 1*4 - 2) / 2, ",0]",
		"image_button[0,0;1,0.8;", core.formspec_escape(defaulttexturedir), "start_icon.png;pstart;]",
		"image_button[1,0;1,0.8;", core.formspec_escape(defaulttexturedir), "prev_icon.png;pback;]",
		"style[pagenum;border=false]",
		"button[2,0;2,0.8;pagenum;", tonumber(cur_page), " / ", tonumber(dlgdata.pagemax), "]",
		"image_button[4,0;1,0.8;", core.formspec_escape(defaulttexturedir), "next_icon.png;pnext;]",
		"image_button[5,0;1,0.8;", core.formspec_escape(defaulttexturedir), "end_icon.png;pend;]",
		"container_end[]", -- page nav end

		-- Bottom-right: updating
		"container[", W - 3, ",0]",
		"style[status,downloading,queued;border=false]",
	})

	if contentdb.number_downloading > 0 then
		formspec[#formspec + 1] = "button[0,0;3,0.8;downloading;"
		if #contentdb.download_queue > 0 then
			formspec[#formspec + 1] = fgettext("$1 downloading,\n$2 queued",
					contentdb.number_downloading, #contentdb.download_queue)
		else
			formspec[#formspec + 1] = fgettext("$1 downloading...", contentdb.number_downloading)
		end
		formspec[#formspec + 1] = "]"
	else
		local num_avail_updates = 0
		for i=1, #contentdb.packages_full do
			local package = contentdb.packages_full[i]
			if package.path and package.installed_release < package.release then
				num_avail_updates = num_avail_updates + 1
			end
		end

		if num_avail_updates == 0 then
			formspec[#formspec + 1] = "button[0,0;3,0.8;status;"
			formspec[#formspec + 1] = fgettext("No updates")
			formspec[#formspec + 1] = "]"
		else
			formspec[#formspec + 1] = "style[update_all;bgcolor=#0284c7;textcolor=white;font=bold]"
			formspec[#formspec + 1] = "style[update_all:hovered;bgcolor=#0369a1]"
			formspec[#formspec + 1] = "button[0,0;3,0.8;update_all;"
			-- TRANSLATORS: $1 = number of available updates
			formspec[#formspec + 1] = fgettext("Update All [$1]", num_avail_updates)
			formspec[#formspec + 1] = "]"
		end
	end

	formspec[#formspec + 1] = "container_end[]" -- updating end
	formspec[#formspec + 1] = "container_end[]" -- bottom strip end

	if #contentdb.packages == 0 then
		formspec[#formspec + 1] = "label[4,4.75;"
		formspec[#formspec + 1] = fgettext("No results")
		formspec[#formspec + 1] = "]"
	end

	-- download/queued tooltips always have the same message
	local tooltip_colors = ";#dff6f5;#302c2e]"
	formspec[#formspec + 1] = "tooltip[downloading;" .. fgettext("Downloading...") .. tooltip_colors
	-- TRANSLATORS: A download is queued
	formspec[#formspec + 1] = "tooltip[queued;" .. fgettext("Queued") .. tooltip_colors

	-- Full Width Packages Browser Box (Layer 2) - Premium 9-sliced rounded corner container
	formspec[#formspec + 1] = ("background9[0,1.2;%f,%f;button_hover_semitrans.png;false;6,6]"):format(W, H - 2.225)

	formspec[#formspec + 1] = "container[0,1.425]"

	local cell_spacing, columns, cell_w, cell_h = fit_cells(num_per_page, {
		x = W,
		y = H - 1.425 - 0.25 - 0.8
	})
	local img_w = cell_h * 3 / 2

	-- Use as much of the available space as possible (so no padding on the
	-- right/bottom), but don't quite allow the text to touch the border.
	local text_w = cell_w - img_w - 0.25 - 0.025
	local text_h = cell_h - 0.25 - 0.025

	local start_idx = (cur_page - 1) * num_per_page + 1
	for i=start_idx, math.min(#contentdb.packages, start_idx+num_per_page-1) do
		local package = contentdb.packages[i]
		local text = core.colorize("#38bdf8", package.title) ..
			core.colorize("#94a3b8", " by " .. package.author) .. "\n" ..
			package.short_description

		table.insert_all(formspec, {
			"container[",
			(cell_w + cell_spacing) * ((i - start_idx) % columns),
			",",
			(cell_h + cell_spacing) * math.floor((i - start_idx) / columns),
			"]",

			-- Modern greyish blue translucent card cells using 9-slice rounded corners
			"background9[0,0;", cell_w, ",", cell_h, ";button_hover_semitrans.png;false;6,6]",

			-- image (nested cleanly in rounded border offset)
			"image[0.1,0.1;", img_w - 0.2, ",", cell_h - 0.2, ";",
				core.formspec_escape(get_screenshot(package, package.thumbnail, 2)), "]",

			"label[", img_w + 0.15, ",0.25;", text_w, ",", text_h, ";",
				core.formspec_escape(text), "]",

			-- Add a tooltip in case the label overflows and the short description is cut off.
			"tooltip[", img_w + 0.15, ",0.25;", text_w, ",", text_h, ";",
				-- Text in tooltips doesn't wrap automatically, so we do it manually to
				-- avoid everything being one long line.
				core.formspec_escape(core.wrap_text(package.short_description, 80)), "]",

			"style[view_", i, ";border=false]",
			"style[view_", i, ":hovered;bgimg=", core.formspec_escape(defaulttexturedir .. "button_hover_semitrans.png"), "]",
			"style[view_", i, ":pressed;bgimg=", core.formspec_escape(defaulttexturedir .. "button_press_semitrans.png"), "]",
			"button[0,0;", cell_w, ",", cell_h, ";view_", i, ";]",
		})

		if package.featured then
			table.insert_all(formspec, {
				--[[ TRANSLATORS: A 'featured' package in ContentDB is a package that is
				more prominently displayed than other packages ]]
				"tooltip[0,0;0.8,0.8;", fgettext("Featured"), "]",
				"image[0.2,0.2;0.4,0.4;", core.formspec_escape(defaulttexturedir .. "server_favorite.png"), "]",
			})
		end

		table.insert_all(formspec, {
			"container[", cell_w - 0.625,",", 0.125, "]",
		})

		if package.downloading then
			table.insert_all(formspec, {
				"animated_image[0,0;0.5,0.5;downloading;", core.formspec_escape(defaulttexturedir .. "cdb_downloading.png"),
					";3;400;;]",
			})
		elseif package.queued then
			table.insert_all(formspec, {
				"image[0,0;0.5,0.5;", core.formspec_escape(defaulttexturedir .. "cdb_queued.png"), "]",
			})
		elseif package.path then
			if package.installed_release < package.release then
				table.insert_all(formspec, {
					"image[0,0;0.5,0.5;", core.formspec_escape(defaulttexturedir .. "cdb_update.png"), "]",
				})
			else
				table.insert_all(formspec, {
					"image[0.1,0.1;0.3,0.3;", core.formspec_escape(defaulttexturedir .. "checkbox_64.png"), "]",
				})
			end
		end

		table.insert_all(formspec, {
			"container_end[]",
			"container_end[]",
		})
	end

	formspec[#formspec + 1] = "container_end[]"
	formspec[#formspec + 1] = "container_end[]"

	return table.concat(formspec)
end


local function handle_submit(this, fields)
	if fields.btn_categories_dropdown then
		this.data.categories_expanded = not this.data.categories_expanded
		return true
	end

	if fields.search or fields.key_enter_field == "search_string" then
		search_string = fields.search_string:trim()
		cur_page = 1
		contentdb.filter_packages(search_string, filter_type)
		return true
	end

	if fields.clear then
		search_string = ""
		cur_page = 1
		contentdb.filter_packages("", filter_type)
		return true
	end

	if fields.back then
		this:delete()
		return true
	end

	if fields.pstart then
		cur_page = 1
		return true
	end

	if fields.pend then
		cur_page = this.data.pagemax
		return true
	end

	if fields.pnext then
		cur_page = cur_page + 1
		if cur_page > this.data.pagemax then
			cur_page = 1
		end
		return true
	end

	if fields.pback then
		if cur_page == 1 then
			cur_page = this.data.pagemax
		else
			cur_page = cur_page - 1
		end
		return true
	end

	for _, pair in ipairs(filter_type_names) do
		if fields[pair[1]] then
			filter_type = pair[2]
			cur_page = 1
			contentdb.filter_packages(search_string, filter_type)
			this.data.categories_expanded = false
			return true
		end
	end

	if fields.update_all then
		for i=1, #contentdb.packages_full do
			local package = contentdb.packages_full[i]
			if package.path and package.installed_release < package.release and
					not (package.downloading or package.queued) then
				contentdb.queue_download(package, contentdb.REASON_UPDATE)
			end
		end
		return true
	end

	local num_per_page = this.data.num_per_page
	local start_idx = (cur_page - 1) * num_per_page + 1
	assert(start_idx ~= nil)
	for i=start_idx, math.min(#contentdb.packages, start_idx+num_per_page-1) do
		local package = contentdb.packages[i]
		assert(package)

		if fields["view_" .. i] or fields["title_" .. i] or fields["author_" .. i] then
			local dlg = create_package_dialog(package)
			dlg:set_parent(this)
			this:hide()
			dlg:show()
			return true
		end
	end

	return false
end


local function handle_events(event)
	if event == "DialogShow" then
		-- Don't show the header image behind the dialog.
		mm_game_theme.set_engine(true)

		-- If ContentDB is already loaded, auto-install packages here.
		do_auto_install()

		return true
	end

	if event == "WindowInfoChange" then
		ui.update()
		return true
	end

	return false
end


--- Creates a ContentDB dialog.
---
--- @param type string | nil
--- Sets initial package filter. "game", "mod", "txp" or nil (no filter).
--- @param install_spec table | nil
--- ContentDB ID of package as returned by pkgmgr.get_contentdb_id().
--- Sets package to install or update automatically.
function create_contentdb_dlg(type, install_spec)
	search_string = ""
	cur_page = 1
	filter_type = type

	-- Keep the old auto_install_spec if the caller doesn't specify one.
	if install_spec then
		auto_install_spec = install_spec
	end

	load()

	local dlg = dialog_create("contentdb",
			get_formspec,
			handle_submit,
			handle_events)
	dlg.data.num_per_page = calculate_num_per_page()
	dlg.data.categories_expanded = false
	return dlg
end
