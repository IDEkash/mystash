-- Creative Composition Interface (CCI)
-- Built-in Core Engine & API (Multiplayer-Safe Per-Player Sessions & CSS Class Integration)

cci = {
	sessions = {},
}

-- Session helper
function cci.get_session(player_name)
	if not cci.sessions[player_name] then
		cci.sessions[player_name] = {
			player_name = player_name,
			objects = {},
			next_id = 1,
			is_dirty = false,
			view_id = "cci_view_" .. player_name,
			active = false,
			styles = {}, -- List of injected custom CSS stylesheet strings
		}

		-- Helper methods directly on the session object
		local Session = cci.sessions[player_name]

		function Session:create_object(options)
			return cci.create_object(player_name, options)
		end

		function Session:destroy()
			local to_destroy = {}
			for id, obj in pairs(self.objects) do
				table.insert(to_destroy, obj)
			end
			for _, obj in ipairs(to_destroy) do
				obj:destroy()
			end
			cci.sessions[player_name] = nil
		end
	end
	return cci.sessions[player_name]
end

-- Inject a custom global CSS stylesheet string for a player's WebView session
function cci.inject_style(player_name, css_string)
	local session = cci.get_session(player_name)
	table.insert(session.styles, css_string)
	session.is_dirty = true
end

local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM .. "cci" .. DIR_DELIM

dofile(gamepath .. "object.lua")
dofile(gamepath .. "easytools.lua")
dofile(gamepath .. "runtime.lua")
dofile(gamepath .. "tests.lua")
