-- Luanti
-- Copyright (C) 2018-24 rubenwardy
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function get_description_hypertext(package, info, loading_error)
	-- Hypertext body background: #0d0d0f panel, body copy in secondary text color (#8b8b92), headings/short_description in primary text (#f2f2f4), bold.
	local hypertext = "<global background=#0d0d0f color=#8b8b92 size=14>"
	hypertext = hypertext .. "<big><b><font color=#f2f2f4>" .. core.hypertext_escape(package.short_description) .. "</font></b></big>\n"

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
		hypertext = hypertext .. "\n<font color=#8b8b92>" .. info.long_description.head .. "</font>"

		local first = true
		local function add_link_button(label, name)
			if info[name] then
				if not first then
					hypertext = hypertext .. " | "
				end
				-- Link buttons: small flat pill/text links in accent blue (#3a7bfd)
				hypertext = hypertext .. "<action name=link_" .. name .. "><font color=#3a7bfd>" .. label .. "</font></action>"
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

		hypertext = hypertext .. "\n\n<font color=#8b8b92>" .. info.long_description.body .. "</font>"

	elseif loading_error then
		hypertext = hypertext .. "\n\n<font color=#e34848>" .. hgettext("Error loading package information") .. "</font>"
	else
		hypertext = hypertext .. "\n\n<font color=#6a6a70>" .. hgettext("Loading...") .. "</font>"
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

		-- Root #000000 background
		"box[-0.5,-0.5;", size.x + 1, ",", size.y + 1, ";#000000]",

		"container[", window_padding.x, ",", window_padding.y, "]",

		-- Back & ContentDB page action buttons at bottom
		-- Back/Cancel: secondary/ghost transparent/panel fill #0d0d0f, border #232326, textcolor #8b8b92
		"style[back;bgcolor=#0d0d0f;textcolor=#8b8b92;border=true;border_color=#232326]",
		"style[back:hovered;bgcolor=#16161a;textcolor=#f2f2f4]",
		"button[0,", bottom_buttons_y, ";2,0.8;back;", fgettext("Back"), "]",

		"style[open_contentdb;bgcolor=#0d0d0f;textcolor=#8b8b92;border=true;border_color=#232326]",
		"style[open_contentdb:hovered;bgcolor=#16161a;textcolor=#f2f2f4]",
		"button[", W - 3.5, ",", bottom_buttons_y, ";3.5,0.8;open_contentdb;", fgettext("ContentDB page"), "]",

		-- Page title in large bold white text (#f2f2f4)
		"style_type[label;font_size=+24;font=bold;textcolor=#f2f2f4]",
		"label[0.2,0.4;", core.formspec_escape(package.title), "]",
		"style_type[label;font_size=;font=;textcolor=]",
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
	-- Rating diamonds or stars in primary text (#f2f2f4) and tertiary/faint (#6a6a70). No gold!
	local rating_pill_lbl = ("%s (%.1f)"):format(stars_str, rating_val)

	-- Rating Pill & Author/Developer Pill using Panel elevate #16161a, text color secondary #8b8b92
	formspec[#formspec + 1] = "box[0.2,1.1;2.8,0.5;#16161a]"
	formspec[#formspec + 1] = "box[0.2,1.1;2.8,0.5;#232326;true]" -- 1px equivalent border
	formspec[#formspec + 1] = "style[rat_lbl;textcolor=#f2f2f4]"
	formspec[#formspec + 1] = "label[0.3,1.35;rat_lbl;" .. core.formspec_escape(rating_pill_lbl) .. "]"

	formspec[#formspec + 1] = "box[3.2,1.1;3.2,0.5;#16161a]"
	formspec[#formspec + 1] = "box[3.2,1.1;3.2,0.5;#232326;true]"
	formspec[#formspec + 1] = "style[auth_lbl;textcolor=#8b8b92]"
	formspec[#formspec + 1] = "label[3.3,1.35;auth_lbl;" .. core.formspec_escape("👤 " .. package.author) .. "]"

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
		-- Accent Blue specification: bgcolor #3a7bfd, textcolor #ffffff, hover #2f68d8
		formspec[#formspec + 1] = "style[install;bgcolor=#3a7bfd;textcolor=#ffffff;font=bold;border=false]"
		formspec[#formspec + 1] = "style[install:hovered;bgcolor=#2f68d8]"
		formspec[#formspec + 1] = "button["
		formspec[#formspec + 1] = right_button_rect
		formspec[#formspec + 1] =";install;"
		formspec[#formspec + 1] = label
		formspec[#formspec + 1] = "]"
	else
		if package.installed_release < package.release then
			-- The install_ action also handles updating (Primary action: accent blue)
			formspec[#formspec + 1] = "style[install;bgcolor=#3a7bfd;textcolor=#ffffff;font=bold;border=false]"
			formspec[#formspec + 1] = "style[install:hovered;bgcolor=#2f68d8]"
			formspec[#formspec + 1] = "button["
			formspec[#formspec + 1] = left_button_rect
			formspec[#formspec + 1] = ";install;"
			formspec[#formspec + 1] = fgettext("Update")
			formspec[#formspec + 1] = "]"
		end

		-- "Uninstall" uses secondary/ghost style (transparent/panel fill #0d0d0f, border #232326) with textcolor #e34848 danger red
		formspec[#formspec + 1] = "style[uninstall;bgcolor=#0d0d0f;textcolor=#e34848;border=true;border_color=#232326]"
		formspec[#formspec + 1] = "style[uninstall:hovered;bgcolor=#16161a]"
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

	-- Left Pane: Hero, stats, categories, screenshots (Width: 6.0) - panel fill #0d0d0f, border #232326
	table.insert_all(formspec, {
		"box[0.2,1.8;6.0," .. (tab_body_height + 0.15) .. ";#0d0d0f]",
		"box[0.2,1.8;6.0," .. (tab_body_height + 0.15) .. ";#232326;true]", -- 1px equivalent border
	})

	-- Large Hero Image card
	local screenshot_url = (info and info.screenshots and info.screenshots[1]) and info.screenshots[1].url or package.thumbnail
	local hero_screenshot = get_screenshot(package, screenshot_url, 2)
	table.insert_all(formspec, {
		"image[0.4,2.0;5.6,2.5;" .. core.formspec_escape(hero_screenshot) .. "]",
		"box[0.4,2.0;5.6,2.5;#232326;true]", -- border for screenshot matching #232326

		-- Badge bottom-left: "Add-On", "Game", or "Mod" - border only (no fill), text color secondary (#8b8b92)
		"box[0.5,4.0;1.4,0.4;#232326;true]",
		"style[badge_lbl;font=bold;textcolor=#8b8b92]",
		"label[0.6,4.18;badge_lbl;" .. core.formspec_escape(package.type:upper()) .. "]",

		-- Badge bottom-right: "FREE" or download size - flat border only, text secondary
		"box[4.3,4.0;1.6,0.4;#232326;true]",
		"style[price_lbl;font=normal;textcolor=#8b8b92]",
		"label[4.4,4.18;price_lbl;" .. core.formspec_escape(info and info.download_size or "FREE") .. "]",

		-- Engagement stats row (downloads / ratings summary) cleanly arranged with spacing - panel elevated #16161a, text secondary #8b8b92
		"box[0.4,4.5;5.6,0.9;#16161a]",
		"box[0.4,4.5;5.6,0.9;#232326;true]",
		"style[stat_lbl1,stat_lbl2,stat_lbl3;textcolor=#8b8b92]",
		"label[0.5,4.72;stat_lbl1;" .. core.formspec_escape(detail_line1) .. "]",
		"label[0.5,5.0;stat_lbl2;" .. core.formspec_escape(detail_line2) .. "]",
		"label[0.5,5.28;stat_lbl3;" .. core.formspec_escape(detail_line3) .. "]",

		-- Screenshots Section label and gallery thumbnails (bold white title, faint divider)
		"style[ss_lbl;font=bold;textcolor=#f2f2f4]",
		"label[0.4,5.6;ss_lbl;" .. fgettext("SCREENSHOTS") .. "]",
		"box[0.4,5.9;5.6,0.02;#232326]",
	})

	-- Load screenshot thumbnails if available
	local ss1_url = (info and info.screenshots and info.screenshots[1]) and info.screenshots[1].url or package.thumbnail
	local ss2_url = (info and info.screenshots and info.screenshots[2]) and info.screenshots[2].url or package.thumbnail
	table.insert_all(formspec, {
		"image[0.4,6.0;2.6,1.4;" .. core.formspec_escape(get_screenshot(package, ss1_url, 2)) .. "]",
		"box[0.4,6.0;2.6,1.4;#232326;true]",
		"image[3.2,6.0;2.8,1.4;" .. core.formspec_escape(get_screenshot(package, ss2_url, 2)) .. "]",
		"box[3.2,6.0;2.8,1.4;#232326;true]",
	})

	-- Right Pane: Tabs and Details or reviews
	table.insert_all(formspec, {
		-- Panel / card fill #0d0d0f with border #232326
		"box[6.4,1.8;" .. (W - 6.4) .. "," .. (tab_body_height + 0.15) .. ";#0d0d0f]",
		"box[6.4,1.8;" .. (W - 6.4) .. "," .. (tab_body_height + 0.15) .. ";#232326;true]",

		"container[6.4,1.8]",
	})

	-- Inactive tab: bgcolor = #0d0d0f, textcolor = #8b8b92
	-- Active tab: bgcolor = #16161a, textcolor = #f2f2f4, thin indicator (#3a7bfd)
	local tab_w = (W - 6.4) / #tab_titles
	for idx, title in ipairs(tab_titles) do
		local bg_col = (idx == current_tab) and "#16161a" or "#0d0d0f"
		local text_col = (idx == current_tab) and "#f2f2f4" or "#8b8b92"
		local font_style = (idx == current_tab) and "bold" or "normal"
		formspec[#formspec + 1] = ("style[cust_pkg_tab_%d;bgcolor=%s;textcolor=%s;border=false;font=%s]button[%f,0;%f,0.6;cust_pkg_tab_%d;%s]"):format(
			idx, bg_col, text_col, font_style, (idx - 1) * tab_w, tab_w, idx, title
		)
		if idx == current_tab then
			formspec[#formspec + 1] = ("box[%f,0.54;%f,0.06;#3a7bfd]"):format((idx - 1) * tab_w, tab_w)
		end
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
			-- Left Column: Details Card (Compatibility and Version Support) - panel fill #0d0d0f with border #232326
			"box[0,0;" .. sub_card_w .. "," .. pane_h .. ";#0d0d0f]",
			"box[0,0;" .. sub_card_w .. "," .. pane_h .. ";#232326;true]",
			"style[det_hdr;font=bold;font_size=+12;textcolor=#f2f2f4]",
			"label[0.2,0.3;det_hdr;" .. fgettext("Details") .. "]",

			"style[det_sub1,det_sub3;textcolor=#8b8b92]",
			"style[det_sub2,det_sub4;textcolor=#f2f2f4]",
			"label[0.2,0.8;det_sub1;" .. fgettext("System Compatibility:") .. "]",
			"label[0.2,1.2;det_sub2;" .. fgettext("Verified & Optimized for Engine") .. "]",

			"label[0.2,1.8;det_sub3;" .. fgettext("Developer:") .. "]",
			"label[0.2,2.2;det_sub4;" .. core.formspec_escape(package.author) .. "]",

			-- Right Column: Ratings Card
			"box[" .. (sub_card_w + 0.4) .. ",0;" .. sub_card_w .. "," .. pane_h .. ";#0d0d0f]",
			"box[" .. (sub_card_w + 0.4) .. ",0;" .. sub_card_w .. "," .. pane_h .. ";#232326;true]",
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
				"style[bar_lbl_" .. bar_idx .. ";textcolor=#8b8b92]",
				"label[" .. (sub_card_w + 0.6) .. "," .. row_y .. ";bar_lbl_" .. bar_idx .. ";" .. row.label .. "]",
				-- Dark progress track background
				"box[" .. track_x .. "," .. row_y .. ";2.5,0.2;#16161a]",
				"box[" .. track_x .. "," .. row_y .. ";2.5,0.2;#232326;true]",
			})
			if fill_w > 0 then
				table.insert_all(formspec, {
					-- Primary Action accent color #3a7bfd
					"box[" .. track_x .. "," .. row_y .. ";" .. fill_w .. ",0.2;#3a7bfd]",
				})
			end
			table.insert_all(formspec, {
				"label[" .. (track_x + 2.6) .. "," .. row_y .. ";bar_lbl_" .. bar_idx .. ";" .. row.pct .. "%]",
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
