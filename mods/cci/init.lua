-- Creative Composition Interface (CCI)
-- Core Engine & Mod API

cci = {
	objects = {},
	next_id = 1,
	is_dirty = false,
}

-- Load sub-modules
local modpath = minetest.get_modpath(minetest.get_current_modname())

dofile(modpath .. "/object.lua")
dofile(modpath .. "/easytools.lua")
dofile(modpath .. "/runtime.lua")
dofile(modpath .. "/tests.lua")

-- Global initialization
minetest.register_on_mods_loaded(function()
	cci.runtime.init()
end)
