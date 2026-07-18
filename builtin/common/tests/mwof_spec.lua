-- Unit and integration tests for Minetek World Object Framework (MWOF)

-- Mock necessary Luanti globally available variables and functions
_G.core = _G.core or {}
core.settings = {
	get = function(self, key)
		if key == "movement_gravity" then return "9.81" end
		return nil
	end,
	get_bool = function(self, key) return false end
}

-- Mock registry of entities, steps, and callbacks
local registered_entities = {}
local registered_globalsteps = {}
local mods_loaded_callbacks = {}
local shutdown_callbacks = {}

core.register_entity = function(name, def)
	registered_entities[name] = def
end

core.register_globalstep = function(func)
	table.insert(registered_globalsteps, func)
end

core.register_on_mods_loaded = function(func)
	table.insert(mods_loaded_callbacks, func)
end

core.register_on_shutdown = function(func)
	table.insert(shutdown_callbacks, func)
end

-- Mock entities
local spawn_count = 0
local spawned_entities = {}
core.add_entity = function(pos, name)
	spawn_count = spawn_count + 1
	local ent_def = registered_entities[name]
	local mock_ref = {
		id = spawn_count,
		pos = {x = pos.x, y = pos.y, z = pos.z},
		vel = {x = 0, y = 0, z = 0},
		acc = {x = 0, y = 0, z = 0},
		properties = {textures = {"blank.png"}},
		is_valid = function(self) return true end,
		get_pos = function(self) return self.pos end,
		get_velocity = function(self) return self.vel end,
		get_acceleration = function(self) return self.acc end,
		get_properties = function(self) return self.properties end,
		set_pos = function(self, p) self.pos = p end,
		set_velocity = function(self, v) self.vel = v end,
		set_acceleration = function(self, a) self.acc = a end,
		set_properties = function(self, props)
			for k, v in pairs(props) do
				self.properties[k] = v
			end
		end,
		remove = function(self) spawned_entities[self.id] = nil end,
	}
	local lua_inst = {}
	if ent_def then
		for k, v in pairs(ent_def) do
			lua_inst[k] = v
		end
	end
	lua_inst.object = mock_ref
	mock_ref.get_luaentity = function() return lua_inst end
	spawned_entities[spawn_count] = mock_ref
	return mock_ref
end

-- Mock world nodes
local world_nodes = {}
local world_metadata = {}
local world_timers = {}

core.get_node = function(pos)
	local k = string.format("%d,%d,%d", pos.x, pos.y, pos.z)
	return world_nodes[k] or {name = "air", param1 = 0, param2 = 0}
end

core.set_node = function(pos, node)
	local k = string.format("%d,%d,%d", pos.x, pos.y, pos.z)
	world_nodes[k] = {name = node.name, param1 = node.param1 or 0, param2 = node.param2 or 0}
end

-- Mock metadata and inventory
core.get_meta = function(pos)
	local k = string.format("%d,%d,%d", pos.x, pos.y, pos.z)
	local meta = world_metadata[k] or { fields = {}, lists = { main = {} } }
	world_metadata[k] = meta

	local mock_inv = {
		get_lists = function(self)
			local lst = {}
			for n, _ in pairs(meta.lists) do table.insert(lst, n) end
			return lst
		end,
		get_list = function(self, name)
			return meta.lists[name] or {}
		end,
		set_list = function(self, name, list)
			local stack_list = {}
			for i, v in ipairs(list) do
				local itemstring = tostring(v)
				stack_list[i] = {
					to_string = function() return itemstring end,
					is_empty = function() return itemstring == "" end
				}
			end
			meta.lists[name] = stack_list
		end
	}

	return {
		to_table = function()
			return {fields = meta.fields}
		end,
		from_table = function(self, t)
			if t then meta.fields = t.fields or {} end
		end,
		get_inventory = function()
			return mock_inv
		end,
	}
end

-- Mock registered nodes
core.registered_nodes = setmetatable({}, {
	__index = function(self, name)
		return { walkable = (name == "default:stone" or name == "default:wood") }
	end
})

core.get_node_timer = function(pos)
	local k = string.format("%d,%d,%d", pos.x, pos.y, pos.z)
	local timer = world_timers[k] or {timeout = 0, elapsed = 0, started = false}
	world_timers[k] = timer
	return {
		is_started = function() return timer.started end,
		get_timeout = function() return timer.timeout end,
		get_elapsed = function() return timer.elapsed end,
		set = function(self, timeout, elapsed)
			timer.timeout = timeout
			timer.elapsed = elapsed
			timer.started = true
		end
	}
end

-- Mock detached inventory
local detached_inventories = {}
core.create_detached_inventory = function(name, callbacks)
	local inv = {
		size = 0,
		lists = {},
		set_size = function(self, list_name, size)
			self.size = size
			self.lists[list_name] = {}
		end
	}
	detached_inventories[name] = inv
	return inv
end

-- Mock active players
local active_players = {}
core.get_connected_players = function()
	return active_players
end

-- Mock secure path check
core.get_worldpath = function() return "." end

-- Load MWOF dependencies & file
_G.DIR_DELIM = "/"
dofile("builtin/common/math.lua")
dofile("builtin/common/vector.lua")
dofile("builtin/common/serialize.lua")
dofile("builtin/game/mwof.lua")

describe("MWOF - Minetek World Object Framework", function()
	before_each(function()
		core.WorldObjectManager.objects = {}
		core.WorldObjectManager.constraints = {}
		world_nodes = {}
		world_metadata = {}
		world_timers = {}
		active_players = {}
	end)

	describe("WorldObject Creation & Wrapping", function()
		it("should create a basic WorldObject", function()
			local obj = core.create_world_object({
				pos = {x = 10, y = 20, z = 30},
				mesh = "ship.glb",
				persistent = true,
			})

			assert.is_not_nil(obj)
			assert.equal("ship.glb", obj.mesh)
			assert.is_true(obj.persistent)
			assert.same({x = 10, y = 20, z = 30}, obj:get_pos())
			assert.is_not_nil(obj.object_ref)
		end)

		it("should wrap an existing entity", function()
			local ent = core.add_entity({x = 5, y = 5, z = 5}, "mwof:object")
			local obj = core.object_from_entity(ent)

			assert.is_not_nil(obj)
			assert.same({x = 5, y = 5, z = 5}, obj:get_pos())
		end)
	end)

	describe("Modular Component System", function()
		it("should support dynamic component addition and removal", function()
			local obj = core.create_world_object({
				pos = {x = 10, y = 10, z = 10},
			})

			-- Initially should not have health component
			assert.is_nil(obj:get_component("health"))

			-- Add health component
			obj:add_component("health")
			local health = obj:get_component("health")
			assert.is_not_nil(health)
			assert.equal(100, health:get_hp(obj))

			-- Set health
			health:set_hp(obj, 50)
			assert.equal(50, health:get_hp(obj))

			-- Remove component
			obj:remove_component("health")
			assert.is_nil(obj:get_component("health"))
		end)

		it("should initialize components listed in creation definition", function()
			local obj = core.create_world_object({
				pos = {x = 0, y = 0, z = 0},
				components = {"inventory", "light", "health"}
			})

			assert.is_not_nil(obj:get_component("inventory"))
			assert.is_not_nil(obj:get_component("light"))
			assert.is_not_nil(obj:get_component("health"))
		end)
	end)

	describe("Node Capture and Exact Restoration", function()
		it("should capture a single node and restore it exactly", function()
			local pos = {x = 12, y = 14, z = 16}
			core.set_node(pos, {name = "default:stone", param1 = 15, param2 = 4})

			-- Set mock metadata
			local meta = core.get_meta(pos)
			meta:from_table({ fields = { owner = "Jules" } })

			-- Set inventory item
			local inv = meta:get_inventory()
			inv:set_list("main", {"default:coal_lump 99"})

			-- Set timer
			local timer = core.get_node_timer(pos)
			timer:set(10.0, 2.5)

			-- Perform node capture
			local obj = core.object_from_node(pos)
			assert.is_not_nil(obj)

			-- The node at the position should now be air
			assert.equal("air", core.get_node(pos).name)

			-- Restore the node
			obj:restore()

			-- Verify node was put back exactly
			local restored_node = core.get_node(pos)
			assert.equal("default:stone", restored_node.name)
			assert.equal(15, restored_node.param1)
			assert.equal(4, restored_node.param2)

			local restored_meta = core.get_meta(pos):to_table()
			assert.equal("Jules", restored_meta.fields.owner)

			local restored_inv = core.get_meta(pos):get_inventory()
			assert.equal("default:coal_lump 99", restored_inv:get_list("main")[1]:to_string())

			local restored_timer = core.get_node_timer(pos)
			assert.is_true(restored_timer:is_started())
			assert.equal(10.0, restored_timer:get_timeout())
		end)

		it("should capture an area of nodes and restore them exactly", function()
			local minp = {x = 1, y = 1, z = 1}
			local maxp = {x = 2, y = 2, z = 2}

			-- Populate area
			for x = 1, 2 do
				for y = 1, 2 do
					for z = 1, 2 do
						local p = {x = x, y = y, z = z}
						core.set_node(p, {name = "default:wood", param1 = 10, param2 = 1})
					end
				end
			end

			-- Capture the nodes
			local obj = core.object_from_area(minp, maxp)
			assert.is_not_nil(obj)

			-- Check that area is cleared
			for x = 1, 2 do
				for y = 1, 2 do
					for z = 1, 2 do
						local p = {x = x, y = y, z = z}
						assert.equal("air", core.get_node(p).name)
					end
				end
			end

			-- Restore area
			obj:restore()

			-- Check that area nodes are completely restored
			for x = 1, 2 do
				for y = 1, 2 do
					for z = 1, 2 do
						local p = {x = x, y = y, z = z}
						assert.equal("default:wood", core.get_node(p).name)
					end
				end
			end
		end)
	end)

	describe("Constraint Solver & Attachments", function()
		it("should solve Fixed constraints", function()
			local obj1 = core.create_world_object({pos = {x = 0, y = 0, z = 0}})
			local obj2 = core.create_world_object({pos = {x = 10, y = 10, z = 10}})

			-- Attach obj1 to obj2 using a Fixed constraint with offset {x=0, y=2, z=0}
			core.WorldObjectManager:add_constraint("Fixed", obj1, obj2, {offset = {x = 0, y = 2, z = 0}})

			-- Run physics step
			core.WorldObjectManager:step(0.1)

			-- obj1 should be pulled to obj2 + offset
			assert.same({x = 10, y = 12, z = 10}, obj1:get_pos())
		end)

		it("should solve Spring constraints", function()
			local obj1 = core.create_world_object({pos = {x = 0, y = 0, z = 0}})
			local obj2 = core.create_world_object({pos = {x = 2, y = 0, z = 0}})

			-- Add spring between obj1 and obj2
			core.WorldObjectManager:add_constraint("Spring", obj1, obj2, {k = 100, rest_length = 1.0})

			-- Simulate physics
			core.WorldObjectManager:step(0.01)

			-- Velocities or accelerations should be affected to pull them together
			assert.is_true(obj1.acc.x > 0)
		end)
	end)

	describe("World Streaming Performance Optimization", function()
		it("should sleep objects that are far from all players and wake them when a player gets close", function()
			local obj = core.create_world_object({pos = {x = 100, y = 0, z = 0}})

			-- No players online, should go to sleep
			core.WorldObjectManager:step(0.1)
			assert.is_true(obj.is_sleeping)
			assert.is_nil(obj.object_ref)

			-- Player joins far away (90 blocks)
			local mock_player = {
				get_pos = function() return {x = 10, y = 0, z = 0} end
			}
			table.insert(active_players, mock_player)

			-- Should still sleep
			core.WorldObjectManager:step(0.1)
			assert.is_true(obj.is_sleeping)

			-- Player moves close (20 blocks)
			mock_player.get_pos = function() return {x = 80, y = 0, z = 0} end

			-- Should wake up and restore visual entity representation
			core.WorldObjectManager:step(0.1)
			assert.is_false(obj.is_sleeping)
			assert.is_not_nil(obj.object_ref)
		end)
	end)

	describe("Persistence Save & Restore Database", function()
		it("should serialize and deserialize persistent world objects", function()
			local obj = core.create_world_object({
				pos = {x = 50, y = 100, z = 150},
				persistent = true,
				mesh = "cube.glb"
			})

			-- Save persistent data
			core.WorldObjectManager:save_persistent_data()

			-- Clear manager objects
			core.WorldObjectManager.objects = {}

			-- Reload persistent data
			core.WorldObjectManager:load_persistent_data()

			-- Verify loaded object
			local restored_obj = core.WorldObjectManager.objects[obj.id]
			assert.is_not_nil(restored_obj)
			assert.same({x = 50, y = 100, z = 150}, restored_obj:get_pos())
			assert.equal("cube.glb", restored_obj.mesh)
		end)
	end)
end)
