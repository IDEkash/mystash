local builtin_shared = ...
local make_registration = builtin_shared.make_registration

-- Dimensions API

core.dimension = core.dimension or {}
core.dim = core.dimension

-- Set globals for convenience (as per API proposal)
dim = core.dimension

core.registered_on_enter_worlds, core.register_on_enter_world = make_registration()
core.registered_on_leave_worlds, core.register_on_leave_world = make_registration()
core.registered_on_dimension_createds, core.register_on_dimension_created = make_registration()
core.registered_on_dimension_deleteds, core.register_on_dimension_deleted = make_registration()
core.registered_on_dimension_loads, core.register_on_dimension_load = make_registration()
core.registered_on_dimension_unloads, core.register_on_dimension_unload = make_registration()

-- Export to the dimension namespace
core.dimension.register_on_enter_world = core.register_on_enter_world
core.dimension.register_on_leave_world = core.register_on_leave_world
core.dimension.register_on_dimension_created = core.register_on_dimension_created
core.dimension.register_on_dimension_deleted = core.register_on_dimension_deleted
core.dimension.register_on_dimension_load = core.register_on_dimension_load
core.dimension.register_on_dimension_unload = core.register_on_dimension_unload

-- Proposal aliases
core.dimension.register_on_join = core.register_on_enter_world
core.dimension.register_on_leave = core.register_on_leave_world
core.dimension.register_on_created = core.register_on_dimension_created
core.dimension.register_on_deleted = core.register_on_dimension_deleted
core.dimension.register_on_load = core.register_on_dimension_load
core.dimension.register_on_unload = core.register_on_dimension_unload

-- Lifecycle event handlers
core.register_on_mods_loaded(function()
	core.run_callbacks(core.registered_on_dimension_loads, 0, core.get_worldpath())
end)

core.register_on_shutdown(function()
	core.run_callbacks(core.registered_on_dimension_unloads, 0, core.get_worldpath())
end)

-- Internal functions to be called by C++
function core.dimension.on_enter_world(player, world_path)
	core.run_callbacks(core.registered_on_enter_worlds, 0, player, world_path)
end

function core.dimension.on_leave_world(player, world_path)
	core.run_callbacks(core.registered_on_leave_worlds, 0, player, world_path)
end
