local builtin_shared = ...
local make_registration = builtin_shared.make_registration

-- Dimensions

core.dimension = core.dimension or {}
core.dim = core.dimension

-- Set globals for convenience (as per API proposal)
dim = core.dimension

core.registered_on_enter_worlds, core.register_on_enter_world = make_registration()
core.registered_on_leave_worlds, core.register_on_leave_world = make_registration()
core.registered_on_dimension_createds, core.register_on_dimension_created = make_registration()
core.registered_on_dimension_deleteds, core.register_on_dimension_deleted = make_registration()

-- Aliases for dim.* namespace as per proposal
core.dimension.register_on_join = core.register_on_enter_world
core.dimension.register_on_leave = core.register_on_leave_world
core.dimension.register_on_created = core.register_on_dimension_created
core.dimension.register_on_deleted = core.register_on_dimension_deleted

-- Internal functions to be called by C++
function core.dimension.on_enter_world(player_name, world_path)
	local player = core.get_player_by_name(player_name)
	core.run_callbacks(core.registered_on_enter_worlds, 0, player, world_path)
end

function core.dimension.on_leave_world(player_name, world_path)
	local player = core.get_player_by_name(player_name)
	core.run_callbacks(core.registered_on_leave_worlds, 0, player, world_path)
end
