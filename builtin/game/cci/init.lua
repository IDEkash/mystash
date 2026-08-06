-- Creative Composition Interface (CCI)
-- Built-in Core Engine & API

cci = {
	objects = {},
	next_id = 1,
	is_dirty = false,
}

local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM .. "cci" .. DIR_DELIM

dofile(gamepath .. "object.lua")
dofile(gamepath .. "easytools.lua")
dofile(gamepath .. "runtime.lua")
dofile(gamepath .. "tests.lua")
