local scriptpath = core.get_builtin_path()
local pausepath = scriptpath.."pause_menu"..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM

-- we're in-game, so no absolute paths are needed
defaulttexturedir = ""

local builtin_shared = {}

assert(loadfile(commonpath .. "register.lua"))(builtin_shared)
assert(loadfile(commonpath .. "menu.lua"))(builtin_shared)
assert(loadfile(pausepath .. "register.lua"))(builtin_shared)
dofile(commonpath .. "settings" .. DIR_DELIM .. "init.lua")

if htmlview and htmlview.is_supported and htmlview.is_supported() then
	local pause_id = "pause:ui"
	local pause_path = core.get_mainmenu_path() .. DIR_DELIM .. "html_ui"

	function core.show_pause_menu()
		htmlview.run_external(pause_id, pause_path, "pause.html")
		htmlview.display(pause_id, {
			visible = true,
			x = "center",
			y = "center",
			width = 440,
			height = 520,
			safe_area = true,
			border_radius = 20,
		})

		htmlview.on_message(pause_id, function(msg_str)
			local data = core.parse_json(msg_str)
			if not data then return end

			if data.action == "request_init_data" then
				local is_sp = pause_menu.is_internal_server()
				local payload = {
					type = "init_data",
					mode = is_sp and "Singleplayer" or "Multiplayer",
					target = is_sp and "local world" or "remote server",
					info_label = "LUANTI PAUSE MENU"
				}
				htmlview.send_json(pause_id, payload)
			elseif data.action == "continue" then
				htmlview.stop(pause_id)
			elseif data.action == "settings" then
				htmlview.stop(pause_id)
				pause_menu.open_settings()
			elseif data.action == "exit_menu" then
				htmlview.stop(pause_id)
				pause_menu.disconnect()
			elseif data.action == "exit_os" then
				htmlview.stop(pause_id)
				pause_menu.exit_to_os()
			end
		end)

		return true
	end
end
