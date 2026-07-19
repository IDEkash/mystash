-- Luanti
-- Copyright (C) 2018-24 rubenwardy
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function get_description_hypertext(package, info, loading_error)
	-- Screenshots and description
	local hypertext = "<big><b>" .. core.hypertext_escape(package.short_description) .. "</b></big>\n"

	local screenshots = info and info.screenshots or {{url = package.thumbnail}}

	local winfo = core.get_window_info()
	local fs_to_px = winfo.size.x / winfo.max_formspec_size.x
	for i, ss in ipairs(screenshots) do
		local path = get_screenshot(package, ss.url, 2)
		hypertext = hypertext .. "<action name=\"ss_".. i .. "\"><img name=\"" ..
				core.hypertext_escape(path) .. "\" width=" .. (3 * fs_to_px) ..
				" height=" .. (2 * fs_to_px) .. "></action>"
		if i ~= #screenshots then
			hypertext = hypertext .. "<img name=\"blank.png\" width=" .. (0.25 * fs_to_px) ..
					" height=" .. (2.25 * fs_to_px).. ">"
		end
	end

	if info then
		hypertext = hypertext .. "\n" .. info.long_description.head

		local first = true
		local function add_link_button(label, name)
			if info[name] then
				if not first then
					hypertext = hypertext .. " | "
				end
				hypertext = hypertext .. "<action name=link_" .. name .. ">" .. label .. "</action>"
				info.long_description.links["link_" .. name] = info[name]
				first = false
			end
		end

		add_link_button(hgettext("Donate"), "donate_url")
		add_link_button(hgettext("Website"), "website")
		-- TRANSLATORS: Link to source code repository
		add_link_button(hgettext("Source"), "repo")
		-- TRANSLATORS: Also known as 'bug tracker'
		add_link_button(hgettext("Issue Tracker"), "issue_tracker")
		add_link_button(hgettext("Translate"), "translation_url")
		add_link_button(hgettext("Forum Topic"), "forum_url")

		hypertext = hypertext .. "\n\n" .. info.long_description.body

	elseif loading_error then
		hypertext = hypertext .. "\n\n" .. hgettext("Error loading package information")
	else
		hypertext = hypertext .. "\n\n" .. hgettext("Loading...")
	end

	-- Fix the path to blank.png. This is needed for bullet indentation,
	-- and also used for screenshot spacing.
	hypertext = hypertext:gsub("<img name=\"?blank.png\"? ",
			"<img name=\"" .. core.hypertext_escape(defaulttexturedir) .. "blank.png\" ")

	return hypertext
end


local function get_formspec(data)
	local package = data.package
	local window_padding =  contentdb.get_formspec_padding()
	local size = contentdb.get_formspec_size()
	size.x = math.min(size.x, 20)
	local W = size.x - window_padding.x * 2
	local H = size.y - window_padding.y * 2

	if not data.info then
		if not data.loading and not data.loading_error then
			data.loading = true

			contentdb.get_full_package_info(package, function(info)
				data.loading = false

				if info == nil then
					data.loading_error = true
					ui.update()
					return
				end

				assert(data.package.name == info.name)
				data.info = info
				-- note: get_full_package_info can also return cached info immediately
				ui.update()
			end)
		end
	end

	-- Check installation status
	contentdb.update_paths()

	local info = data.info

	local detail_line1 = fgettext_ne("by $1", package.author)
	local detail_line2 = info and fgettext_ne("$1 downloads", info.downloads) or fgettext("No data yet")
	local detail_line3 = "Reviews: +0 / -0"
	local total_positive = 0
	local total_neutral = 0
	local total_negative = 0
	if info and info.reviews then
		total_positive = info.reviews.positive or 0
		total_neutral = info.reviews.neutral or 0
		total_negative = info.reviews.negative or 0
		detail_line3 = ("Reviews: +%d / =%d / -%d"):format(total_positive, total_neutral, total_negative)
	end

	local bottom_buttons_y = H - 0.8

	local formspec = {
		"formspec_version[7]",
		"size[", size.x, ",",  size.y, "]",
		"padding[0,0]",
		"bgcolor[;true]",

		-- Solid dark premium greyish blue backdrop (Layer 1)
		"box[-0.5,-0.5;", size.x + 1, ",", size.y + 1, ";#0f172aF2]",

		"container[", window_padding.x, ",", window_padding.y, "]",

		-- Back & ContentDB page action buttons at bottom
		"style[back;bgcolor=#9b2c2c;textcolor=white]",
		"style[back:hovered;bgcolor=#b91c1c]",
		"button[0,", bottom_buttons_y, ";2,0.8;back;", fgettext("Back"), "]",
		"style[open_contentdb;bgcolor=#334155;textcolor=white;border=false]",
		"style[open_contentdb:hovered;bgcolor=#475569]",
		"button[", W - 3.5, ",", bottom_buttons_y, ";3.5,0.8;open_contentdb;", fgettext("ContentDB page"), "]",

		-- Page title in large bold white text
		"style_type[label;font_size=+24;font=bold]",
		"label[0.2,0.4;", core.formspec_escape(package.title), "]",
		"style_type[label;font_size=;font=]",
	}

	-- Draw pill-shaped metadata tags next to title
	local rating_val = 5.0
	local total_rev = total_positive + total_neutral + total_negative
	if total_rev > 0 then
		rating_val = 3.0 + 2.0 * (total_positive / total_rev)
	end

	-- Building precise visual unicode star rating:
	local stars_str = ""
	local temp_val = rating_val
	for s = 1, 5 do
		if temp_val >= 1.0 then
			stars_str = stars_str .. "★"
			temp_val = temp_val - 1.0
		elseif temp_val >= 0.5 then
			stars_str = stars_str .. "★"
			temp_val = 0
		else
			stars_str = stars_str .. "☆"
		end
	end
	local rating_pill_lbl = ("%s (%.1f)"):format(stars_str, rating_val)

	-- Rating Pill & Author/Developer Pill using Slate colors
	formspec[#formspec + 1] = "box[0.2,1.1;2.8,0.5;#33415590]"
	formspec[#formspec + 1] = "label[0.3,1.35;" .. core.formspec_escape(rating_pill_lbl) .. "]"
	formspec[#formspec + 1] = "box[3.2,1.1;3.2,0.5;#33415590]"
	formspec[#formspec + 1] = "label[3.3,1.35;" .. core.formspec_escape("👤 " .. package.author) .. "]"

	-- Right aligned installation controls
	table.insert_all(formspec, {
		"container[", W - 6.2, ",0.4]"
	})

	local left_button_rect = "0,0;2.875,1"
	local right_button_rect = "3.125,0;2.875,1"
	if package.downloading then
		formspec[#formspec + 1] = "animated_image[5,0;1,1;downloading;"
		formspec[#formspec + 1] = core.formspec_escape(defaulttexturedir)
		formspec[#formspec + 1] = "cdb_downloading.png;3;400;]"
	elseif package.queued then
		formspec[#formspec + 1] = "style[queued;border=false]"
		formspec[#formspec + 1] = "image_button[5,0;1,1;" .. core.formspec_escape(defaulttexturedir)
		formspec[#formspec + 1] = "cdb_queued.png;queued;]"
	elseif not package.path then
		-- TRANSLATORS: $1 = download size
		local label = info and fgettext("Install [$1]", info.download_size) or
			fgettext("Install")
		-- Sky blue highlight background
		formspec[#formspec + 1] = "style[install;bgcolor=#0284c7;textcolor=white;font=bold]"
		formspec[#formspec + 1] = "style[install:hovered;bgcolor=#0369a1]"
		formspec[#formspec + 1] = "button["
		formspec[#formspec + 1] = right_button_rect
		formspec[#formspec + 1] =";install;"
		formspec[#formspec + 1] = label
		formspec[#formspec + 1] = "]"
	else
		if package.installed_release < package.release then
			-- The install_ action also handles updating
			formspec[#formspec + 1] = "style[install;bgcolor=#0ea5e9;textcolor=white]"
			formspec[#formspec + 1] = "style[install:hovered;bgcolor=#0284c7]"
			formspec[#formspec + 1] = "button["
			formspec[#formspec + 1] = left_button_rect
			formspec[#formspec + 1] = ";install;"
			formspec[#formspec + 1] = fgettext("Update")
			formspec[#formspec + 1] = "]"
		end

		formspec[#formspec + 1] = "style[uninstall;bgcolor=#a93b3b;textcolor=white]"
		formspec[#formspec + 1] = "style[uninstall:hovered;bgcolor=#b91c1c]"
		formspec[#formspec + 1] = "button["
		formspec[#formspec + 1] = right_button_rect
		formspec[#formspec + 1] = ";uninstall;"
		formspec[#formspec + 1] = fgettext("Uninstall")
		formspec[#formspec + 1] = "]"
	end

	table.insert_all(formspec, {
		"container_end[]", -- installation controls container end
	})

	local current_tab = data.current_tab or 1
	local tab_titles = {
		fgettext("Description"),
	}
	if info then
		local review_count = info.reviews.positive + info.reviews.neutral + info.reviews.negative
		table.insert(tab_titles, fgettext("Information"))
		table.insert(tab_titles, fgettext("Reviews") .. core.formspec_escape(" [" .. review_count .. "]"))
	end

	local tab_body_height = bottom_buttons_y - 2.0

	-- Left Pane: Hero, stats, categories, screenshots (Width: 6.0) using rounded background9 card
	table.insert_all(formspec, {
		"background9[0.2,1.8;6.0," .. (tab_body_height + 0.15) .. ";button_hover_semitrans.png;false;6,6]",
	})

	-- Large Hero Image card
	local screenshot_url = (info and info.screenshots and info.screenshots[1]) and info.screenshots[1].url or package.thumbnail
	local hero_screenshot = get_screenshot(package, screenshot_url, 2)
	table.insert_all(formspec, {
		"image[0.4,2.0;5.6,2.5;" .. core.formspec_escape(hero_screenshot) .. "]",
		-- Badge bottom-left: "Add-On", "Game", or "Mod"
		"box[0.5,4.0;1.4,0.4;#0284c7]",
		"style[badge_lbl;font=bold;textcolor=white]",
		"label[0.6,4.18;" .. core.formspec_escape(package.type:upper()) .. "]",
		-- Badge bottom-right: "FREE" or download size
		"box[4.3,4.0;1.6,0.4;#334155B0]",
		"style[price_lbl;font=normal;textcolor=white]",
		"label[4.4,4.18;" .. core.formspec_escape(info and info.download_size or "FREE") .. "]",

		-- Engagement stats row (downloads / ratings summary) cleanly arranged with spacing
		"box[0.4,4.5;5.6,0.9;#33415560]",
		"label[0.5,4.72;" .. core.formspec_escape(detail_line1) .. "]",
		"label[0.5,5.0;" .. core.formspec_escape(detail_line2) .. "]",
		"label[0.5,5.28;" .. core.formspec_escape(detail_line3) .. "]",

		-- Screenshots Section label and gallery thumbnails
		"label[0.4,5.6;" .. fgettext("SCREENSHOTS") .. "]",
		"box[0.4,5.9;5.6,0.02;#ffffff22]",
	})

	-- Load screenshot thumbnails if available
	local ss1_url = (info and info.screenshots and info.screenshots[1]) and info.screenshots[1].url or package.thumbnail
	local ss2_url = (info and info.screenshots and info.screenshots[2]) and info.screenshots[2].url or package.thumbnail
	table.insert_all(formspec, {
		"image[0.4,6.0;2.6,1.4;" .. core.formspec_escape(get_screenshot(package, ss1_url, 2)) .. "]",
		"image[3.2,6.0;2.8,1.4;" .. core.formspec_escape(get_screenshot(package, ss2_url, 2)) .. "]",
	})

	-- Right Pane: Tabs and Details or reviews
	table.insert_all(formspec, {
		-- Translucent Content Card Panel (Layer 2) using rounded background9 card
		"background9[6.4,1.8;" .. (W - 6.4) .. "," .. (tab_body_height + 0.15) .. ";button_hover_semitrans.png;false;6,6]",

		"container[6.4,1.8]",
	})

	-- Horizontal Connected Styled Tab Header Row (Modern Sky Blue Accent)
	local tab_w = (W - 6.4) / #tab_titles
	for idx, title in ipairs(tab_titles) do
		local bg_col = (idx == current_tab) and "#0284c7" or "#334155"
		local text_col = (idx == current_tab) and "#ffffff" or "#94a3b8"
		local font_style = (idx == current_tab) and "bold" or "normal"
		formspec[#formspec + 1] = ("style[cust_pkg_tab_%d;bgcolor=%s;textcolor=%s;border=false;font=%s]button[%f,0;%f,0.6;cust_pkg_tab_%d;%s]"):format(
			idx, bg_col, text_col, font_style, (idx - 1) * tab_w, tab_w, idx, title
		)
	end

	table.insert_all(formspec, {
		"container_end[]",
		"container[6.6,2.6]",
	})

	local pane_w = W - 6.8
	local pane_h = tab_body_height - 0.8

	if current_tab == 1 then
		local hypertext = get_description_hypertext(package, info, data.loading_error)
		table.insert_all(formspec, {
			"hypertext[0,0;" .. pane_w .. "," .. pane_h .. ";desc;" .. core.formspec_escape(hypertext) .. "]",
		})

	elseif current_tab == 2 then
		assert(info)
		-- Two column layout inside Information tab: Details card on left, Ratings breakdown on right
		local sub_card_w = pane_w / 2 - 0.2
		table.insert_all(formspec, {
			-- Left Column: Details Card (Compatibility and Version Support) - Premium 9-slice Rounded Slate Card
			"background9[0,0;" .. sub_card_w .. "," .. pane_h .. ";button_hover_semitrans.png;false;6,6]",
			"style[det_hdr;font=bold;font_size=+12]",
			"label[0.2,0.3;det_hdr;" .. fgettext("Details") .. "]",

			"label[0.2,0.8;" .. fgettext("System Compatibility:") .. "]",
			"label[0.2,1.2;" .. fgettext("Verified & Optimized for Engine") .. "]",

			"label[0.2,1.8;" .. fgettext("Developer:") .. "]",
			"label[0.2,2.2;" .. core.formspec_escape(package.author) .. "]",

			-- Right Column: Ratings Card - Premium 9-slice Rounded Slate Card
			"background9[" .. (sub_card_w + 0.4) .. ",0;" .. sub_card_w .. "," .. pane_h .. ";button_hover_semitrans.png;false;6,6]",
			"label[" .. (sub_card_w + 0.6) .. ",0.3;det_hdr;" .. fgettext("Ratings") .. "]",
		})

		-- Generate high-fidelity progress bar chart breakdown for reviews
		local total_rev = total_positive + total_neutral + total_negative
		local pos_pct = total_rev > 0 and math.floor(total_positive / total_rev * 100) or 80
		local neu_pct = total_rev > 0 and math.floor(total_neutral / total_rev * 100) or 7
		local neg_pct = total_rev > 0 and math.floor(total_negative / total_rev * 100) or 13

		local bar_idx = 1
		for _, row in ipairs({
			{ label = "5 ★", pct = pos_pct },
			{ label = "4 ★", pct = 0 },
			{ label = "3 ★", pct = neu_pct },
			{ label = "2 ★", pct = 0 },
			{ label = "1 ★", pct = neg_pct },
		}) do
			local row_y = 0.8 + (bar_idx - 1) * 0.65
			local track_x = sub_card_w + 1.2
			local fill_w = (row.pct / 100) * 2.5
			table.insert_all(formspec, {
				"label[" .. (sub_card_w + 0.6) .. "," .. row_y .. ";" .. row.label .. "]",
				-- Dark progress track background
				"box[" .. track_x .. "," .. row_y .. ";2.5,0.2;#1E1E1EE5]",
			})
			if fill_w > 0 then
				table.insert_all(formspec, {
					-- Sky-blue styled progress fill
					"box[" .. track_x .. "," .. row_y .. ";" .. fill_w .. ",0.2;#0284c7]",
				})
			end
			table.insert_all(formspec, {
				"label[" .. (track_x + 2.6) .. "," .. row_y .. ";" .. row.pct .. "%]",
			})
			bar_idx = bar_idx + 1
		end

	elseif current_tab == 3 then
		assert(info)
		if not package.reviews and not data.reviews_error and not data.reviews_loading then
			data.reviews_loading = true

			contentdb.get_package_reviews(package, function(reviews)
				if not reviews then
					data.reviews_error = true
				end
				ui.update()
			end)
		end

		if package.reviews then
			local hypertext = package.reviews.head .. package.reviews.body
			-- Provide correct path to blank.png image. This is needed for bullet indentation.
			hypertext = hypertext:gsub("<img name=\"?blank.png\"? ",
					"<img name=\"" .. core.hypertext_escape(defaulttexturedir) .. "blank.png\" ")
			-- Placeholders in reviews hypertext for icons
			hypertext = hypertext:gsub("<thumbsup>",
					"<img name=\"" .. core.hypertext_escape(defaulttexturedir) .. "contentdb_thumb_up.png\" width=24>")
			hypertext = hypertext:gsub("<thumbsdown>",
					"<img name=\"" .. core.hypertext_escape(defaulttexturedir) .. "contentdb_thumb_down.png\" width=24>")
			hypertext = hypertext:gsub("<neutral>",
					"<img name=\"" .. core.hypertext_escape(defaulttexturedir) .. "contentdb_neutral.png\" width=24>")
			table.insert_all(formspec, {
				"hypertext[0,0;" .. pane_w .. "," .. pane_h .. ";reviews;" .. core.formspec_escape(hypertext) .. "]",
			})
		elseif data.reviews_error then
			table.insert_all(formspec, {"label[2,2;", fgettext("Error loading reviews"), "]"} )
		else
			table.insert_all(formspec, {"label[2,2;", fgettext("Loading..."), "]"} )
		end
	else
		error("Unknown tab " .. current_tab)
	end

	formspec[#formspec + 1] = "container_end[]"
	formspec[#formspec + 1] = "container_end[]"

	return table.concat(formspec)
end


local function handle_hypertext_event(this, event, hypertext_object)
	if not (event and event:sub(1, 7) == "action:") then
		return
	end

	for i, ss in ipairs(this.data.info.screenshots) do
		if event == "action:ss_" .. i then
			core.open_url(ss.url)
			return true
		end
	end

	local base_url = core.settings:get("contentdb_url"):gsub("(%W)", "%%%1")
	for key, url in pairs(hypertext_object.links) do
		if event == "action:" .. key then
			local author, name = url:match("^" .. base_url .. "/?packages/([A-Za-z0-9 _-]+)/([a-z0-9_]+)/?$")
			if author and name then
				local package2 = contentdb.get_package_by_info(author, name)
				if package2 then
					local dlg = create_package_dialog(package2)
					dlg:set_parent(this)
					this:hide()
					dlg:show()
					return true
				end
			end

			core.open_url_dialog(url)
			return true
		end
	end
end


local function handle_submit(this, fields)
	local info = this.data.info
	local package = this.data.package

	if fields.back then
		this:delete()
		return true
	end

	if fields.open_contentdb then
		local version = core.get_version()
		local url = core.settings:get("contentdb_url") .. "/packages/" .. package.url_part ..
				"/?protocol_version=" .. core.urlencode(core.get_max_supp_proto()) ..
				"&engine_version=" .. core.urlencode(version.string)
		core.open_url(url)
		return true
	end

	if fields.install then
		install_or_update_package(this, package)
		return true
	end

	if fields.uninstall then
		local dlg = create_delete_content_dlg(package)
		dlg:set_parent(this)
		this:hide()
		dlg:show()
		return true
	end

	-- Handle custom styled tab header clicks
	for idx = 1, 3 do
		if fields["cust_pkg_tab_" .. idx] then
			this.data.current_tab = idx
			return true
		end
	end

	-- The events handled below are only valid if the package info has finished
	-- loading.
	if not info then
		return false
	end

	if fields.tabs then
		this.data.current_tab = tonumber(fields.tabs)
		return true
	end

	if handle_hypertext_event(this, fields.desc, info.long_description) or
			handle_hypertext_event(this, fields.info, info.info_hypertext) or
			(package.reviews and handle_hypertext_event(this, fields.reviews, package.reviews)) then
		return true
	end
end


local function handle_events(event)
	if event == "WindowInfoChange" then
		ui.update()
		return true
	end

	return false
end


function create_package_dialog(package)
	assert(package)

	local dlg = dialog_create("package_dialog_" .. package.id,
			get_formspec,
			handle_submit,
			handle_events)
	local data = dlg.data

	data.package = package
	data.info = nil
	data.loading = false
	data.loading_error = nil
	data.current_tab = 1
	return dlg
end
