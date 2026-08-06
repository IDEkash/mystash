-- Creative Composition Interface (CCI)
-- Built-in Core Engine & API (Multiplayer-Safe Per-Player Sessions)

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
		}

		-- Helper methods directly on the session object
		local Session = cci.sessions[player_name]

		function Session:create_object(options)
			return cci.create_object(player_name, options)
		end

		function Session:destroy()
			for _, obj in pairs(self.objects) do
				obj:destroy()
			end
			cci.sessions[player_name] = nil
		end
	end
	return cci.sessions[player_name]
end

local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM .. "cci" .. DIR_DELIM

dofile(gamepath .. "object.lua")
dofile(gamepath .. "easytools.lua")
dofile(gamepath .. "runtime.lua")
dofile(gamepath .. "tests.lua")
