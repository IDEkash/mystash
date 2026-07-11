-- Luanti Monochromatic HTML Main Menu Handler

local ui_id = "mainmenu:ui"
local ui_path = core.get_mainmenu_path() .. DIR_DELIM .. "html_ui"

local function send_init_data()
	-- Worlds List
	local raw_worlds = core.get_worlds()
	local worlds = {}
	for i, w in ipairs(raw_worlds) do
		table.insert(worlds, {
			name = w.name,
			gameid = w.gameid,
			raw_index = i
		})
	end

	-- Installed Games List
	local games = {}
	for _, g in ipairs(pkgmgr.games) do
		table.insert(games, {
			id = g.id,
			title = g.title
		})
	end

	-- Server List (Favorites & top public servers)
	local favs = serverlistmgr.get_favorites() or {}
	local servers = {}
	for _, f in ipairs(favs) do
		table.insert(servers, {
			name = f.name or "Favorite Server",
			address = f.address,
			port = tostring(f.port or 30000),
			clients = "Favorite"
		})
	end

	if serverlistmgr.servers and #serverlistmgr.servers > 0 then
		for i = 1, math.min(10, #serverlistmgr.servers) do
			local s = serverlistmgr.servers[i]
			table.insert(servers, {
				name = s.name or "Public Server",
				address = s.address,
				port = tostring(s.port or 30000),
				clients = tostring(s.clients or 0) .. "/" .. tostring(s.clients_max or 50)
			})
		end
	end

	-- Settings values
	local settings = {
		creative_mode = core.settings:get_bool("creative_mode"),
		enable_damage = core.settings:get_bool("enable_damage"),
		smooth_lighting = core.settings:get_bool("smooth_lighting"),
		enable_shaders = core.settings:get_bool("enable_shaders"),
		mip_map = core.settings:get_bool("mip_map"),
		enable_sound = core.settings:get_bool("enable_sound"),
		enable_node_highlighting = core.settings:get_bool("enable_node_highlighting"),
		viewing_range = tonumber(core.settings:get("viewing_range")) or 100,
		sound_volume = math.floor((tonumber(core.settings:get("sound_volume")) or 0.8) * 100),
		player_name = core.settings:get("name") or "singleplayer",
	}

	local payload = {
		type = "init_data",
		version = core.get_version(),
		worlds = worlds,
		games = games,
		servers = servers,
		settings = settings
	}

	htmlview.send_json(ui_id, payload)
end

-- Initialize and run HTMLView overlay
htmlview.run_external(ui_id, ui_path, "index.html")
htmlview.display(ui_id, {
	visible = true,
	x = "center",
	y = "center",
	width = "fullscreen",
	height = "fullscreen",
	safe_area = true,
})

-- Listen for communication from WebView
htmlview.on_message(ui_id, function(msg_str)
	local data = core.parse_json(msg_str)
	if not data then return end

	if data.action == "request_init_data" then
		send_init_data()
	elseif data.action == "create_world" then
		core.create_world(data.name, data.gameid)
		send_init_data()
	elseif data.action == "delete_world" then
		local raw_worlds = core.get_worlds()
		for i, w in ipairs(raw_worlds) do
			if w.name == data.name then
				core.delete_world(i)
				break
			end
		end
		send_init_data()
	elseif data.action == "play_world" then
		local raw_worlds = core.get_worlds()
		local raw_idx = nil
		for i, w in ipairs(raw_worlds) do
			if w.name == data.name then
				raw_idx = i
				break
			end
		end
		if raw_idx then
			gamedata.selected_world = raw_idx
			gamedata.singleplayer = true

			core.settings:set_bool("creative_mode", data.creative)
			core.settings:set_bool("enable_damage", data.damage)

			menu_worldmt(raw_idx, "creative_mode", tostring(data.creative))
			menu_worldmt(raw_idx, "enable_damage", tostring(data.damage))

			htmlview.stop(ui_id)
			core.start()
		end
	elseif data.action == "connect_server" then
		gamedata.singleplayer = false
		gamedata.address = data.address
		gamedata.port = tonumber(data.port) or 30000
		gamedata.playername = data.name
		gamedata.password = data.password

		core.settings:set("name", data.name)

		htmlview.stop(ui_id)
		core.start()
	elseif data.action == "save_setting" then
		local val = data.value
		if data.key == "sound_volume" then
			val = val / 100
		end
		core.settings:set(data.key, tostring(val))
	elseif data.action == "quit" then
		core.close()
	end
end)
