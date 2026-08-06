-- CCI Demo Mod (Chapter 5 Composition Examples - Multiplayer-Safe & Pure CSS Stylesheets)
-- Demonstrates how to compose complex interactive widgets (Buttons, Toggles, Windows)
-- using the core CCI systems and pure CSS stylesheets.

local function msg(name, text)
	minetest.chat_send_player(name, "[CCI Demo] " .. text)
end

-- Inject the core CSS Stylesheet rules for the UI (Chapter 3 / CSS integration)
local function inject_demo_css(player_name)
	cci.inject_style(player_name, [[
		/* Window Theme */
		.cci-window {
			background: #1a1e29;
			border: 1px solid rgba(255,255,255,0.1);
			box-shadow: 0 10px 25px rgba(0,0,0,0.5);
			padding: 10px;
			border-radius: 16px;
			color: #ffffff;
			transition: background 0.3s ease;
		}
		.cci-window.dark-theme {
			background: #0d1117;
		}

		/* Window Titlebar Header */
		.cci-window-header {
			background: rgba(255,255,255,0.05);
			display: flex;
			align-items: center;
			padding: 0 10px;
			cursor: move;
			border-radius: 8px;
		}
		.cci-window-title {
			color: #ffffff;
			font-weight: bold;
			font-size: 14px;
		}

		/* Button Theme */
		.cci-btn {
			background: linear-gradient(180deg, #4d90fe, #357ae8);
			border: 1px solid #2f5bb7;
			color: #ffffff;
			display: flex;
			align-items: center;
			justify-content: center;
			font-weight: bold;
			cursor: pointer;
			box-shadow: 0 2px 4px rgba(0,0,0,0.2);
			transition: transform 0.1s ease, background 0.1s ease;
			border-radius: 12px;
		}
		.cci-btn.pressed {
			transform: scale(0.95);
			background: linear-gradient(180deg, #357ae8, #2251a2);
		}
		.cci-btn.close-btn {
			background: #ff3b30;
			border: none;
		}

		/* Textbox Label Theme */
		.cci-btn-label {
			font-size: 16px;
			text-shadow: 0 1px 1px rgba(0,0,0,0.3);
		}

		/* Toggle Theme */
		.cci-toggle-track {
			background: #ccc;
			border: 1px solid #bbb;
			transition: background 0.2s ease;
			cursor: pointer;
			border-radius: 18px;
		}
		.cci-toggle-track.active {
			background: #4cd964;
		}
		.cci-toggle-handle {
			background: #ffffff;
			box-shadow: 0 1px 3px rgba(0,0,0,0.4);
			transition: transform 0.2s ease;
			border-radius: 50%;
		}
		.cci-toggle-handle.active {
			transform: translate3d(34px, 0px, 0px);
		}

		/* Informative Label */
		.cci-info-txt {
			color: #9aa7c7;
			font-size: 14px;
			line-height: 1.5;
		}
		.cci-label-txt {
			color: #ffffff;
			font-size: 14px;
		}
	]])
end

-- Compose a beautiful Button
local function create_button(player_name, id, text, x, y, width, height, onClick)
	-- 1. Create the container rectangle
	local btn = cci.easytools.create_rounded_rectangle(player_name, width, height, 12, {
		id = id,
		x = x,
		y = y,
	})
	btn:add_class("cci-btn")

	-- 2. Create the internal Textbox element (hierarchy support)
	local label = cci.create_object(player_name, {
		id = id .. "_label",
		type = "textbox",
	})
	label:add_class("cci-btn-label")
	label:set_style("content", text or "Click Me")
	btn:add_child(label)

	-- 3. Register Functionable Services for interaction
	btn:on("press", function(self)
		self:add_class("pressed")
	end)

	btn:on("release", function(self)
		self:remove_class("pressed")
		if onClick then
			onClick(self)
		end
	end)

	return btn
end

-- Compose a beautiful Toggle Switch (Chapter 5 Example)
local function create_toggle(player_name, id, x, y, onChange)
	-- Track Object
	local track = cci.easytools.create_rounded_rectangle(player_name, 70, 36, 18, {
		id = id,
		x = x,
		y = y,
	})
	track:add_class("cci-toggle-track")

	-- Handle Object
	local handle = cci.easytools.create_circle(player_name, 14, {
		id = id .. "_handle",
		x = 4,
		y = 4,
	})
	handle:add_class("cci-toggle-handle")
	track:add_child(handle)

	local is_on = false
	track:on("press", function(self)
		is_on = not is_on
		if is_on then
			self:add_class("active")
			handle:add_class("active")
		else
			self:remove_class("active")
			handle:remove_class("active")
		end
		if onChange then
			onChange(is_on)
		end
	end)

	return track
end

-- Compose a beautiful Window containing content (Chapter 5 Composition)
local function create_window(player_name, id, title, x, y, width, height)
	-- Window Background
	local win = cci.easytools.create_rounded_rectangle(player_name, width, height, 16, {
		id = id,
		x = x,
		y = y,
	})
	win:add_class("cci-window")

	-- Window Header / Titlebar (Drag handler support)
	local header = cci.easytools.create_rounded_rectangle(player_name, width - 20, 40, 8, {
		id = id .. "_header",
		x = 10,
		y = 10,
	})
	header:add_class("cci-window-header")
	win:add_child(header)

	-- Title text
	local title_txt = cci.create_object(player_name, {
		id = id .. "_title",
		type = "textbox",
	})
	title_txt:add_class("cci-window-title")
	title_txt:set_style("content", title)
	header:add_child(title_txt)

	-- Drag Event Registration to move the window (Action Attributes / Transform Update)
	local start_x, start_y
	header:on("drag_start", function(self, data)
		start_x = win.transform.x
		start_y = win.transform.y
	end)

	header:on("dragging", function(self, data)
		if start_x and start_y then
			win:set_pos(start_x + data.dx, start_y + data.dy)
		end
	end)

	-- Close Button inside header (Hierarchy + Runtime Destruction)
	local close_btn = create_button(player_name, id .. "_close", "X", width - 40, 10, 30, 30, function()
		win:destroy() -- Completely destroys window and all children recursively (Chapter 4)
	end)
	close_btn:add_class("close-btn")
	win:add_child(close_btn)

	return win
end

-- Chat command to launch the CCI Demo
minetest.register_chatcommand("cci_demo", {
	params = "start | stop",
	description = "Start/Stop the CCI system demo",
	privs = { interact = true },
	func = function(name, param)
		param = param:trim()
		local session = cci.get_session(name)

		if param == "stop" then
			-- Destroy player's active UI
			session:destroy()
			msg(name, "CCI Demo stopped.")
			return
		end

		-- Clean existing objects in session
		for id, obj in pairs(session.objects) do
			obj:destroy()
		end

		-- 1. Inject the pure CSS styles into the player session!
		inject_demo_css(name)

		-- 2. Create a modern Window container
		local win = create_window(name, "demo_win", "Creative Composition Interface (CCI)", 100, 100, 450, 350)

		-- 3. Compose interactive widgets inside the Window container
		-- Add a simple informative text label
		local info = cci.create_object(name, {
			id = "demo_info",
			type = "textbox",
			x = 20,
			y = 70,
		})
		info:add_class("cci-info-txt")
		info:set_style("content", "Welcome to CCI! Drag this window from the titlebar. Use the button to teleport or the toggle to switch mode.")
		win:add_child(info)

		-- Add a Toggle Switch
		local toggle_label = cci.create_object(name, {
			id = "toggle_label",
			type = "textbox",
			x = 20,
			y = 150,
		})
		toggle_label:add_class("cci-label-txt")
		toggle_label:set_style("content", "Modern Dark Mode Toggle:")
		win:add_child(toggle_label)

		local toggle = create_toggle(name, "demo_toggle", 240, 140, function(state_on)
			if state_on then
				win:add_class("dark-theme")
				msg(name, "Mode: Dark theme enabled via CSS class injection!")
			else
				win:remove_class("dark-theme")
				msg(name, "Mode: Standard theme enabled via CSS class removal!")
			end
		end)
		win:add_child(toggle)

		-- Add an action button
		local action_btn = create_button(name, "demo_action", "Teleport Randomly", 20, 220, 200, 45, function()
			local player = minetest.get_player_by_name(name)
			if player then
				player:set_pos({
					x = math.random(-100, 100),
					y = player:get_pos().y + 5,
					z = math.random(-100, 100)
				})
				msg(name, "You have been teleported by CCI Functionable Services!")
			end
		end)
		win:add_child(action_btn)

		msg(name, "CCI Demo started! A fully composable UI has been initialized using pure CSS stylesheets.")
	end
})
