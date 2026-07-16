-- Luanti
-- Copyright (C) 2014 sapier
-- SPDX-License-Identifier: LGPL-2.1-or-later


local current_game, singleplayer_refresh_gamebar
local valid_disabled_settings = {
	["enable_damage"]=true,
	["creative_mode"]=true,
	["enable_server"]=true,
}

-- Name and port stored to persist when updating the formspec
local current_name = core.settings:get("name")
local current_port = core.settings:get("port")

-- Currently chosen game in gamebar for theming and filtering
function current_game()
	local gameid = core.settings:get("menu_last_game")
	local game = gameid and pkgmgr.find_by_gameid(gameid)
	-- Fall back to first game installed if one exists.
	if not game and #pkgmgr.games > 0 then

		-- If devtest is the first game in the list and there is another
		-- game available, pick the other game instead.
		local picked_game
		if pkgmgr.games[1].id == "devtest" and #pkgmgr.games > 1 then
			picked_game = 2
		else
			picked_game = 1
		end

		game = pkgmgr.games[picked_game]
		gameid = game.id
		core.settings:set("menu_last_game", gameid)
	end

	return game
end

-- Apply menu changes from given game
function apply_game(game)
	core.settings:set("menu_last_game", game.id)
	menudata.worldlist:set_filtercriteria(game.id)

	mm_game_theme.set_game(game)

	local index = filterlist.get_current_index(menudata.worldlist,
		tonumber(core.settings:get("mainmenu_last_selected_world")))
	if not index or index < 1 then
		local selected = core.get_textlist_index("sp_worlds")
		if selected ~= nil and selected < #menudata.worldlist:get_list() then
			index = selected
		else
			index = #menudata.worldlist:get_list()
		end
	end
	menu_worldmt_legacy(index)
end

function singleplayer_refresh_gamebar()
	local old_bar = ui.find_by_name("game_button_bar")
	if old_bar ~= nil then
		old_bar:delete()
	end
	return false
end

local function get_disabled_settings(game)
	if not game then
		return {}
	end

	local gameconfig = Settings(game.path .. "/game.conf")
	local disabled_settings = {}
	if gameconfig then
		local disabled_settings_str = (gameconfig:get("disabled_settings") or ""):split()
		for _, value in pairs(disabled_settings_str) do
			local state = false
			value = value:trim()
			if string.sub(value, 1, 1) == "!" then
				state = true
				value = string.sub(value, 2)
			end
			if valid_disabled_settings[value] then
				disabled_settings[value] = state
			else
				core.log("error", "Invalid disabled setting in game.conf: "..tostring(value))
			end
		end
	end
	return disabled_settings
end

local function get_formspec(tabview, name, tabdata)

	-- Point the player to ContentDB when no games are found
	if #pkgmgr.games == 0 then
		local W = tabview.width
		local H = tabview.height

		local hypertext = "<global valign=middle halign=center size=18>" ..
				fgettext_ne("Luanti is a game-creation platform that allows you to play many different games.") .. "\n" ..
				fgettext_ne("Luanti doesn't come with a game by default.") .. " " ..
				fgettext_ne("You need to install a game before you can create a world.")

		local button_y = H * 2/3 - 0.6
		return table.concat({
			"hypertext[0.375,0;", W - 2*0.375, ",", button_y, ";ht;", core.formspec_escape(hypertext), "]",
			"button[5.25,", button_y, ";5,1.2;game_open_cdb;", fgettext("Install a game"), "]"})
	end

	local retval = "style[play;bgcolor=#467832;textcolor=white;font=bold]" ..
			"style[world_create;bgcolor=#43464b;textcolor=white]" ..
			"style[world_configure;bgcolor=#43464b;textcolor=white]" ..
			"style[world_delete;bgcolor=#9b2c2c;textcolor=white]"

	-- Background card boxes representing two tables with borders
	retval = retval ..
			"box[0.15,0.15;4.8,6.8;#00000060]" ..
			"box[5.05,0.15;10.3,6.8;#00000060]"

	local index = core.get_textlist_index("sp_worlds") or filterlist.get_current_index(menudata.worldlist,
				tonumber(core.settings:get("mainmenu_last_selected_world"))) or 0

	local list = menudata.worldlist:get_list()
	-- When changing tabs to a world list with fewer entries, the last index is selected (visually).
	-- However, the formspec fields lag behind, thus 'index > #list' can be a valid choice.
	local world = list and list[math.min(index, #list)]
	local game

	if world then
		game = pkgmgr.find_by_gameid(world.gameid)
	else
		game = current_game()
	end
	local disabled_settings = get_disabled_settings(game)

	-- Header label of the current game (dropdown button)
	retval = retval .. "style[btn_games_dropdown;bgcolor=#43464b;textcolor=white;font=bold;border=false]" ..
			"button[0.35,0.35;4.4,0.8;btn_games_dropdown;" .. core.formspec_escape(game.title .. " ▾") .. "]"

	if tabdata.games_expanded then
		local scroll_h = 5.2
		local total_h = (#pkgmgr.games + 1) * 0.9
		retval = retval .. ("scroll_container[0.35,1.3;4.2,%f;games_scroll;vertical;0.1;0]"):format(scroll_h)
		for i, g in ipairs(pkgmgr.games) do
			local item_y = (i - 1) * 0.9
			local icon = (g.menuicon_path and g.menuicon_path ~= "") and core.formspec_escape(g.menuicon_path) or core.formspec_escape(defaulttexturedir .. "no_screenshot.png")
			retval = retval ..
				("image[0.1,%f;0.6,0.6;%s]"):format(item_y + 0.1, icon) ..
				("style[game_select_%s;bgcolor=#43464b;textcolor=white;border=false]"):format(g.id) ..
				("button[0.8,%f;3.2,0.8;game_select_%s;%s]"):format(item_y, g.id, core.formspec_escape(g.title))
		end
		local plus_y = #pkgmgr.games * 0.9
		local plus_icon = core.formspec_escape(defaulttexturedir .. "plus.png")
		retval = retval ..
			("image[0.1,%f;0.6,0.6;%s]"):format(plus_y + 0.1, plus_icon) ..
			"style[game_open_cdb_dropdown;bgcolor=#467832;textcolor=white;font=bold;border=false]" ..
			("button[0.8,%f;3.2,0.8;game_open_cdb_dropdown;%s]"):format(plus_y, fgettext("Add Game"))
		retval = retval .. "scroll_container_end[]"
		if total_h > scroll_h then
			retval = retval .. ("scrollbar[4.65,1.3;0.2,%f;vertical;games_scroll;%f]"):format(scroll_h, tabdata.games_scroll or 0)
		end
	end

	local creative, damage, host = "", "", ""

	-- Y offsets for game settings checkboxes
	local y = 1.3
	local yo = 0.5625

	if world and not tabdata.games_expanded then
		if disabled_settings["creative_mode"] == nil then
			creative = "checkbox[0,"..y..";cb_creative_mode;".. fgettext("Creative Mode") .. ";" ..
				dump(core.settings:get_bool("creative_mode")) .. "]"
			y = y + yo
		end
		if disabled_settings["enable_damage"] == nil then
			damage = "checkbox[0,"..y..";cb_enable_damage;".. fgettext("Enable Damage") .. ";" ..
				dump(core.settings:get_bool("enable_damage")) .. "]"
			y = y + yo
		end
		if disabled_settings["enable_server"] == nil then
			host = "checkbox[0,"..y..";cb_server;".. fgettext("Host Server") ..";" ..
				dump(core.settings:get_bool("enable_server")) .. "]"
			y = y + yo
		end
	end

	retval = retval ..
			"container[5.25,4.875]" ..
			"button[6.65,0;3.225,0.8;world_create;".. fgettext("New") .. "]"
	if world then
		retval = retval ..
				"button[0,0;3.225,0.8;world_delete;".. fgettext("Delete") .. "]" ..
				"button[3.325,0;3.225,0.8;world_configure;".. fgettext("Select Mods") .. "]"
	end
	retval = retval ..
			"container_end[]" ..
			"container[0.375,0.375]" ..
			creative ..
			damage ..
			host ..
			"container_end[]" ..
			"container[5.25,0.375]" ..
			"label[0,0.2;".. fgettext("Select World:") .. "]"..
			"textlist[0,0.5;9.875,3.9;sp_worlds;" ..
			menu_render_worldlist() ..
			";" .. index .. "]" ..
			"container_end[]"

	if core.settings:get_bool("enable_server") and disabled_settings["enable_server"] == nil then
		retval = retval ..
				"button[5.25,5.925;9.875,0.8;play;".. fgettext("Host Game") .. "]"
		if not tabdata.games_expanded then
			retval = retval ..
					"container[0.375,0.375]" ..
					"checkbox[0,"..y..";cb_server_announce;" .. fgettext("Announce Server") .. ";" ..
					dump(core.settings:get_bool("server_announce")) .. "]"

			-- Reset y so that the text fields always start at the same position,
			-- regardless of whether some of the checkboxes are hidden.
			y = 1.3 + 4 * yo + 0.35

			retval = retval .. "field[0," .. y .. ";4.5,0.75;te_playername;" .. fgettext("Name") .. ";" ..
					core.formspec_escape(current_name) .. "]"

			y = y + 1.15 + 0.25

			retval = retval .. "pwdfield[0," .. y .. ";4.5,0.75;te_passwd;" .. fgettext("Password") .. "]"

			y = y + 1.15 + 0.25

			local bind_addr = core.settings:get("bind_address")
			if bind_addr ~= nil and bind_addr ~= "" then
				retval = retval ..
					"field[0," .. y .. ";3,0.75;te_serveraddr;" .. fgettext("Bind Address") .. ";" ..
					core.formspec_escape(core.settings:get("bind_address")) .. "]" ..
					-- TRANSLATORS: Network port
					"field[3.25," .. y .. ";1.25,0.75;te_serverport;" .. fgettext("Port") .. ";" ..
					core.formspec_escape(current_port) .. "]"
			else
				retval = retval ..
					"field[0," .. y .. ";4.5,0.75;te_serverport;" .. fgettext("Server Port") .. ";" ..
					core.formspec_escape(current_port) .. "]"
			end

			retval = retval .. "container_end[]"
		end
	elseif world then
		retval = retval ..
				"button[5.25,5.925;9.875,0.8;play;" .. fgettext("Play Game") .. "]"
	end

	-- Title on bottom left side of the left pane
	if not tabdata.games_expanded then
		local ver_str = core.get_version().string
		local colored_text = core.colorize("#467832", "Luanti ") .. core.colorize("#ffffff", core.formspec_escape(ver_str))
		retval = retval .. "label[0.35,6.3;" .. colored_text .. "]"
	end

	return retval
end

local function main_button_handler(this, fields, name, tabdata)

	assert(name == "local")

	if fields.btn_games_dropdown then
		tabdata.games_expanded = not tabdata.games_expanded
		return true
	end

	if fields.game_open_cdb_dropdown then
		local maintab = ui.find_by_name("maintab")
		local dlg = create_contentdb_dlg("game")
		dlg:set_parent(maintab)
		maintab:hide()
		dlg:show()
		return true
	end

	for _, g in ipairs(pkgmgr.games) do
		if fields["game_select_" .. g.id] then
			apply_game(g)
			tabdata.games_expanded = false
			return true
		end
	end

	if fields.games_scroll then
		tabdata.games_scroll = core.explode_scrollbar_event(fields.games_scroll).value or tabdata.games_scroll
		return true
	end

	if fields.game_open_cdb then
		local maintab = ui.find_by_name("maintab")
		local dlg = create_contentdb_dlg("game")
		dlg:set_parent(maintab)
		maintab:hide()
		dlg:show()
		return true
	end

	if this.dlg_create_world_closed_at == nil then
		this.dlg_create_world_closed_at = 0
	end

	local world_doubleclick = false

	if fields["te_playername"] then
		current_name = fields["te_playername"]
	end

	if fields["te_serverport"] then
		current_port = fields["te_serverport"]
	end

	if fields["sp_worlds"] ~= nil then
		local event = core.explode_textlist_event(fields["sp_worlds"])
		local selected = core.get_textlist_index("sp_worlds")

		menu_worldmt_legacy(selected)

		if event.type == "DCL" then
			world_doubleclick = true
		end

		if event.type == "CHG" and selected ~= nil then
			core.settings:set("mainmenu_last_selected_world",
				menudata.worldlist:get_raw_index(selected))
			return true
		end
	end

	if menu_handle_key_up_down(fields,"sp_worlds","mainmenu_last_selected_world") then
		return true
	end

	if fields["cb_creative_mode"] then
		core.settings:set("creative_mode", fields["cb_creative_mode"])
		local selected = core.get_textlist_index("sp_worlds")
		menu_worldmt(selected, "creative_mode", fields["cb_creative_mode"])

		return true
	end

	if fields["cb_enable_damage"] then
		core.settings:set("enable_damage", fields["cb_enable_damage"])
		local selected = core.get_textlist_index("sp_worlds")
		menu_worldmt(selected, "enable_damage", fields["cb_enable_damage"])

		return true
	end

	if fields["cb_server"] then
		core.settings:set("enable_server", fields["cb_server"])

		return true
	end

	if fields["cb_server_announce"] then
		core.settings:set("server_announce", fields["cb_server_announce"])
		local selected = core.get_textlist_index("srv_worlds")
		menu_worldmt(selected, "server_announce", fields["cb_server_announce"])

		return true
	end

	if fields["play"] ~= nil or world_doubleclick or fields["key_enter"] then
		local enter_key_duration = core.get_us_time() - this.dlg_create_world_closed_at
		if world_doubleclick and enter_key_duration <= 200000 then -- 200 ms
			this.dlg_create_world_closed_at = 0
			return true
		end

		local selected = core.get_textlist_index("sp_worlds")
		gamedata.selected_world = menudata.worldlist:get_raw_index(selected)

		if selected == nil or gamedata.selected_world == 0 then
			return true
		end

		-- Update last game
		local world = menudata.worldlist:get_raw_element(gamedata.selected_world)
		local game_obj
		if world then
			game_obj = pkgmgr.find_by_gameid(world.gameid)
			core.settings:set("menu_last_game", game_obj.id)
		end

		local disabled_settings = get_disabled_settings(game_obj)
		for k, _ in pairs(valid_disabled_settings) do
			local v = disabled_settings[k]
			if v ~= nil then
				if k == "enable_server" and v == true then
					error("Setting 'enable_server' cannot be force-enabled! The game.conf needs to be fixed.")
				end
				core.settings:set_bool(k, disabled_settings[k])
			end
		end

		if core.settings:get_bool("enable_server") then
			gamedata.playername = fields["te_playername"]
			gamedata.password   = fields["te_passwd"]
			gamedata.port       = fields["te_serverport"]
			gamedata.address    = ""

			core.settings:set("port",gamedata.port)
			if fields["te_serveraddr"] ~= nil then
				core.settings:set("bind_address",fields["te_serveraddr"])
			end
		else
			gamedata.singleplayer = true
		end

		core.start()
		return true
	end

	if fields["world_create"] ~= nil then
		this.dlg_create_world_closed_at = 0
		local create_world_dlg = create_create_world_dlg()
		create_world_dlg:set_parent(this)
		this:hide()
		create_world_dlg:show()
		return true
	end

	if fields["world_delete"] ~= nil then
		local selected = core.get_textlist_index("sp_worlds")
		if selected ~= nil and
			selected <= menudata.worldlist:size() then
			local world = menudata.worldlist:get_list()[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				local index = menudata.worldlist:get_raw_index(selected)
				local delete_world_dlg = create_delete_world_dlg(world.name,index)
				delete_world_dlg:set_parent(this)
				this:hide()
				delete_world_dlg:show()
			end
		end

		return true
	end

	if fields["world_configure"] ~= nil then
		local selected = core.get_textlist_index("sp_worlds")
		if selected ~= nil then
			local configdialog =
				create_configure_world_dlg(
						menudata.worldlist:get_raw_index(selected))

			if (configdialog ~= nil) then
				configdialog:set_parent(this)
				this:hide()
				configdialog:show()
			end
		end

		return true
	end
end

local function on_change(type)
	if type == "ENTER" then
		local game = current_game()
		if game then
			apply_game(game)
		else
			mm_game_theme.set_engine()
		end

		if singleplayer_refresh_gamebar() then
			ui.find_by_name("game_button_bar"):show()
		end
	elseif type == "LEAVE" then
		menudata.worldlist:set_filtercriteria(nil)
		local gamebar = ui.find_by_name("game_button_bar")
		if gamebar then
			gamebar:hide()
		end
	end
end

--------------------------------------------------------------------------------
return {
	name = "local",
	caption = fgettext("Start Game"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
