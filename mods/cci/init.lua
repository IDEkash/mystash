-- Creative Composition Interface (CCI)
-- Mod-based Core Engine & API (Multiplayer-Safe Per-Player Sessions)

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

local modpath = minetest.get_modpath(minetest.get_current_modname())

dofile(modpath .. "/object.lua")
dofile(modpath .. "/easytools.lua")
dofile(modpath .. "/runtime.lua")
dofile(modpath .. "/tests.lua")
