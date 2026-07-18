-- Minetek World Object Framework (MWOF)
-- A universal dynamic object system that complements Luanti's node system.

core.WorldObjectManager = {}
core.world_object_manager = core.WorldObjectManager

local WorldObjectManager = core.WorldObjectManager
WorldObjectManager.objects = {}
WorldObjectManager.constraints = {}
WorldObjectManager.sleep_distance = 64
WorldObjectManager.wake_distance = 48
WorldObjectManager.autosave_interval = 30
WorldObjectManager.last_autosave = 0

local gravity = tonumber(core.settings:get("movement_gravity")) or 9.81

-- Keep track of registered custom components
WorldObjectManager.registered_components = {}

function WorldObjectManager:register_component(name, def)
	self.registered_components[name] = def
end

-- Unique ID generator
local function generate_uuid()
	return string.format("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
		math.random(0, 0xffff), math.random(0, 0xffff),
		math.random(0, 0xffff),
		math.random(0, 0xffff),
		math.random(0, 0xffff),
		math.random(0, 0xffff), math.random(0, 0xffff), math.random(0, 0xffff)
	)
end

-- WorldObject class definition
local WorldObject = {}
WorldObject.__index = WorldObject

function WorldObject:new(def)
	def = def or {}
	local o = {
		id = def.id or generate_uuid(),
		pos = def.pos and vector.new(def.pos) or vector.new(0, 0, 0),
		vel = def.vel and vector.new(def.vel) or vector.new(0, 0, 0),
		acc = def.acc and vector.new(def.acc) or vector.new(0, 0, 0),
		mass = def.mass or 1.0,
		gravity_scale = def.gravity_scale or 1.0,
		friction = def.friction or 0.1,
		bounce = def.bounce or 0.0,
		is_sleeping = false,
		persistent = def.persistent or false,
		captured_node = def.captured_node,
		captured_area = def.captured_area,
		mesh = def.mesh,
		visual = def.visual or (def.mesh and "mesh" or "sprite"),
		textures = def.textures or {"blank.png"},
		visual_size = def.visual_size or {x = 1, y = 1, z = 1},
		collision_shape = def.collision_shape or {type = "box", size = {x = 1, y = 1, z = 1}},
		collision_groups = def.collision_groups or {node = true},
		components = {},
		attachments = {},
		metadata = def.metadata or {},
		callbacks = def.events or {},
		object_ref = nil, -- Underling Luanti ObjectRef
	}
	setmetatable(o, WorldObject)

	-- Register requested components
	if def.components then
		for _, comp_name in ipairs(def.components) do
			o:add_component(comp_name)
		end
	end

	return o
end

function WorldObject:get_id() return self.id end
function WorldObject:get_pos() return self.pos end
function WorldObject:get_velocity() return self.vel end
function WorldObject:get_acceleration() return self.acc end
function WorldObject:get_mass() return self.mass end
function WorldObject:get_gravity() return self.gravity_scale end
function WorldObject:get_friction() return self.friction end
function WorldObject:get_bounce() return self.bounce end

function WorldObject:set_pos(pos)
	self.pos = vector.new(pos)
	if self.object_ref and self.object_ref:is_valid() then
		self.object_ref:set_pos(pos)
	end
	self:trigger_event("on_move")
end

function WorldObject:set_velocity(vel)
	self.vel = vector.new(vel)
	if self.object_ref and self.object_ref:is_valid() then
		self.object_ref:set_velocity(vel)
	end
end

function WorldObject:set_acceleration(acc)
	self.acc = vector.new(acc)
	if self.object_ref and self.object_ref:is_valid() then
		local grav_acc = vector.new(0, -gravity * self.gravity_scale, 0)
		self.object_ref:set_acceleration(vector.add(acc, grav_acc))
	end
end

function WorldObject:set_mass(mass) self.mass = mass end
function WorldObject:set_gravity(gravity_scale)
	self.gravity_scale = gravity_scale
	self:set_acceleration(self.acc)
end
function WorldObject:set_friction(friction) self.friction = friction end
function WorldObject:set_bounce(bounce) self.bounce = bounce end

function WorldObject:apply_force(force)
	local a = vector.divide(vector.new(force), self.mass)
	self:set_acceleration(vector.add(self.acc, a))
end

function WorldObject:apply_impulse(impulse)
	local dv = vector.divide(vector.new(impulse), self.mass)
	self:set_velocity(vector.add(self.vel, dv))
end

function WorldObject:set_sleeping(bool)
	if bool and not self.is_sleeping then
		self:sleep()
	elseif not bool and self.is_sleeping then
		self:wake()
	end
end

-- Component handling
function WorldObject:add_component(name, comp_def)
	if self.components[name] then return end
	local comp = { name = name }
	local global_comp = WorldObjectManager.registered_components[name]
	if global_comp then
		for k, v in pairs(global_comp) do
			comp[k] = v
		end
	end
	if comp_def then
		for k, v in pairs(comp_def) do
			comp[k] = v
		end
	end
	self.components[name] = comp
	if comp.on_init then
		comp:on_init(self)
	end
end

function WorldObject:get_component(name)
	return self.components[name]
end

function WorldObject:remove_component(name)
	local comp = self.components[name]
	if comp then
		if comp.on_destroy then
			comp:on_destroy(self)
		end
		self.components[name] = nil
	end
end

-- Visual handling (wake / sleep)
function WorldObject:wake()
	if not self.is_sleeping and self.object_ref then return end
	self.is_sleeping = false

	-- Spawn internal entity mwof:object
	local ref = core.add_entity(self.pos, "mwof:object")
	if ref then
		self.object_ref = ref
		local lua_entity = ref:get_luaentity()
		lua_entity.world_object_id = self.id

		-- Setup precise collisionbox from collision shape
		local box = {-0.5, -0.5, -0.5, 0.5, 0.5, 0.5}
		if self.collision_shape and self.collision_shape.type == "box" and self.collision_shape.size then
			local sz = self.collision_shape.size
			box = {-sz.x/2, -sz.y/2, -sz.z/2, sz.x/2, sz.y/2, sz.z/2}
		end

		-- Synchronize properties
		ref:set_properties({
			visual = self.visual,
			mesh = self.mesh,
			textures = self.textures,
			visual_size = self.visual_size,
			physical = true,
			collide_with_objects = true,
			collisionbox = box,
		})
		ref:set_velocity(self.vel)

		local grav_acc = vector.new(0, -gravity * self.gravity_scale, 0)
		ref:set_acceleration(vector.add(self.acc, grav_acc))
	end

	self:trigger_event("on_wake")
end

function WorldObject:sleep()
	if self.is_sleeping then return end
	self.is_sleeping = true

	if self.object_ref and self.object_ref:is_valid() then
		self.object_ref:remove()
		self.object_ref = nil
	end

	self:trigger_event("on_sleep")
end

function WorldObject:destroy()
	self:trigger_event("on_destroy")
	if self.object_ref and self.object_ref:is_valid() then
		self.object_ref:remove()
		self.object_ref = nil
	end
	WorldObjectManager.objects[self.id] = nil
end

function WorldObject:trigger_event(event_name, ...)
	local cb = self.callbacks[event_name]
	if cb then
		cb(self, ...)
	end
	-- Check component listeners
	for _, comp in pairs(self.components) do
		if comp[event_name] then
			comp[event_name](comp, self, ...)
		end
	end
end

-- Attachment Support
function WorldObject:attach_to(parent, offset, rotation)
	offset = offset or {x=0, y=0, z=0}
	rotation = rotation or {x=0, y=0, z=0}
	self.attachments[parent] = {offset = offset, rotation = rotation}
	self:trigger_event("on_attach", parent)
end

function WorldObject:detach(parent)
	if parent then
		if self.attachments[parent] then
			self.attachments[parent] = nil
			self:trigger_event("on_detach", parent)
		end
	else
		for p, _ in pairs(self.attachments) do
			self.attachments[p] = nil
			self:trigger_event("on_detach", p)
		end
	end
end

function WorldObject:get_attachments()
	return self.attachments
end

-- Restoration logic for captured nodes
function WorldObject:restore()
	self:trigger_event("on_restore")

	if self.captured_node then
		local node_pos = self.captured_node.pos
		core.set_node(node_pos, {
			name = self.captured_node.name,
			param1 = self.captured_node.param1,
			param2 = self.captured_node.param2
		})
		-- Restore metadata
		if self.captured_node.meta then
			local meta = core.get_meta(node_pos)
			meta:from_table(self.captured_node.meta)
		end
		-- Restore node inventory lists
		if self.captured_node.inventory then
			local meta = core.get_meta(node_pos)
			local inv = meta:get_inventory()
			for list_name, list_data in pairs(self.captured_node.inventory) do
				inv:set_list(list_name, list_data)
			end
		end
		-- Restore timers
		if self.captured_node.timer then
			local timer = core.get_node_timer(node_pos)
			timer:set(self.captured_node.timer.timeout, self.captured_node.timer.elapsed)
		end
	end

	if self.captured_area then
		for _, node in ipairs(self.captured_area) do
			local abs_pos = vector.add(self.pos, node.rel_pos)
			core.set_node(abs_pos, {
				name = node.name,
				param1 = node.param1,
				param2 = node.param2
			})
			if node.meta then
				local meta = core.get_meta(abs_pos)
				meta:from_table(node.meta)
			end
			if node.inventory then
				local meta = core.get_meta(abs_pos)
				local inv = meta:get_inventory()
				for list_name, list_data in pairs(node.inventory) do
					inv:set_list(list_name, list_data)
				end
			end
			if node.timer then
				local timer = core.get_node_timer(abs_pos)
				timer:set(node.timer.timeout, node.timer.elapsed)
			end
		end
	end

	self:destroy()
end


-- Core manager methods
function WorldObjectManager:create_world_object(def)
	local obj = WorldObject:new(def)
	self.objects[obj.id] = obj
	obj:wake()
	obj:trigger_event("on_spawn")
	return obj
end

function WorldObjectManager:create_object(def)
	return self:create_world_object(def)
end

function WorldObjectManager:object_from_node(pos)
	local node = core.get_node(pos)
	if node.name == "air" or node.name == "ignore" then
		return nil
	end

	local meta = core.get_meta(pos)
	local meta_table = meta:to_table()
	local inv = meta:get_inventory()
	local inv_table = {}
	if inv then
		for _, list_name in ipairs(inv:get_lists() or {}) do
			local list = inv:get_list(list_name)
			local list_strings = {}
			for i, stack in ipairs(list) do
				list_strings[i] = stack:to_string()
			end
			inv_table[list_name] = list_strings
		end
	end

	local timer = core.get_node_timer(pos)
	local timer_data = nil
	if timer:is_started() then
		timer_data = {
			timeout = timer:get_timeout(),
			elapsed = timer:get_elapsed()
		}
	end

	local captured = {
		pos = vector.new(pos),
		name = node.name,
		param1 = node.param1,
		param2 = node.param2,
		meta = meta_table,
		inventory = inv_table,
		timer = timer_data
	}

	-- Remove the node
	core.set_node(pos, {name = "air"})

	local obj = self:create_world_object({
		pos = pos,
		captured_node = captured,
		visual = "wielditem",
		textures = {node.name},
		visual_size = {x = 0.6, y = 0.6, z = 0.6},
	})

	obj:trigger_event("on_capture")
	return obj
end

function WorldObjectManager:node_to_object(pos)
	return self:object_from_node(pos)
end

function WorldObjectManager:object_from_area(minp, maxp)
	local captured_list = {}
	local center = vector.divide(vector.add(minp, maxp), 2)

	for x = minp.x, maxp.x do
		for y = minp.y, maxp.y do
			for z = minp.z, maxp.z do
				local pos = {x = x, y = y, z = z}
				local node = core.get_node(pos)
				if node.name ~= "air" and node.name ~= "ignore" then
					local meta = core.get_meta(pos)
					local meta_table = meta:to_table()
					local inv = meta:get_inventory()
					local inv_table = {}
					if inv then
						for _, list_name in ipairs(inv:get_lists() or {}) do
							local list = inv:get_list(list_name)
							local list_strings = {}
							for i, stack in ipairs(list) do
								list_strings[i] = stack:to_string()
							end
							inv_table[list_name] = list_strings
						end
					end

					local timer = core.get_node_timer(pos)
					local timer_data = nil
					if timer:is_started() then
						timer_data = {
							timeout = timer:get_timeout(),
							elapsed = timer:get_elapsed()
						}
					end

					table.insert(captured_list, {
						rel_pos = vector.subtract(pos, center),
						name = node.name,
						param1 = node.param1,
						param2 = node.param2,
						meta = meta_table,
						inventory = inv_table,
						timer = timer_data
					})

					-- Remove node
					core.set_node(pos, {name = "air"})
				end
			end
		end
	end

	local obj = self:create_world_object({
		pos = center,
		captured_area = captured_list,
		visual = "mesh",
		mesh = "area_captured.gltf", -- fallback or dynamic model
		textures = {"blank.png"},
	})

	obj:trigger_event("on_capture")
	return obj
end

function WorldObjectManager:capture_nodes(minp, maxp)
	return self:object_from_area(minp, maxp)
end

function WorldObjectManager:object_from_player(player)
	return self:create_world_object({
		pos = player:get_pos(),
		visual = "mesh",
		mesh = player:get_properties().mesh,
		textures = player:get_properties().textures,
		metadata = {player_name = player:get_player_name()},
	})
end

function WorldObjectManager:object_from_entity(entity)
	local lua_entity = entity:get_luaentity()
	return self:create_world_object({
		pos = entity:get_pos(),
		visual = entity:get_properties().visual,
		mesh = entity:get_properties().mesh,
		textures = entity:get_properties().textures,
		metadata = {entity_name = lua_entity and lua_entity.name or "unknown"},
	})
end

function WorldObjectManager:object_from_mesh(mesh_name)
	return self:create_world_object({
		mesh = mesh_name,
		visual = "mesh"
	})
end

-- Constraints management
function WorldObjectManager:add_constraint(type_name, obj1, obj2_or_pos, params)
	local constr = {
		id = generate_uuid(),
		type = type_name,
		obj1 = obj1,
		obj2 = obj2_or_pos,
		params = params or {}
	}
	self.constraints[constr.id] = constr
	return constr
end

function WorldObjectManager:remove_constraint(id)
	self.constraints[id] = nil
end

function WorldObjectManager:step(dtime)
	local players = core.get_connected_players()

	-- 1. Stream checking (Sleep vs Wake)
	for id, obj in pairs(self.objects) do
		local min_dist = math.huge
		for _, player in ipairs(players) do
			local d = vector.distance(obj.pos, player:get_pos())
			if d < min_dist then
				min_dist = d
			end
		end

		if min_dist > self.sleep_distance then
			if not obj.is_sleeping and not obj.persistent then
				obj:sleep()
			end
		elseif min_dist < self.wake_distance then
			if obj.is_sleeping then
				obj:wake()
			end
		end
	end

	-- 2. Synchronize / Integrate Physics for Awake Objects
	for id, obj in pairs(self.objects) do
		if not obj.is_sleeping then
			if obj.object_ref and obj.object_ref:is_valid() then
				-- Keep Lua representation in sync with the engine-simulated entity
				obj.pos = obj.object_ref:get_pos()
				obj.vel = obj.object_ref:get_velocity()
			else
				-- Lightweight manual step when entity is temporarily sleeping/despawned
				local grav_acc = vector.new(0, -gravity * obj.gravity_scale, 0)
				obj.vel = vector.add(obj.vel, vector.multiply(vector.add(obj.acc, grav_acc), dtime))
				obj.vel = vector.multiply(obj.vel, 1.0 - (obj.friction * dtime))
				obj.pos = vector.add(obj.pos, vector.multiply(obj.vel, dtime))
			end
		end
	end

	-- 3. Satisfy constraints programmatically
	for id, c in pairs(self.constraints) do
		local obj1 = c.obj1
		local obj2 = c.obj2
		if obj1 and self.objects[obj1.id] then
			if c.type == "Fixed" then
				local offset = c.params.offset or vector.new(0, 0, 0)
				if type(obj2) == "table" and obj2.get_pos then
					-- Constrained to another WorldObject
					obj1:set_pos(vector.add(obj2:get_pos(), offset))
				else
					-- Constrained to a coordinate
					obj1:set_pos(vector.add(obj2, offset))
				end
			elseif c.type == "Spring" then
				local k = c.params.k or 10
				local rest_length = c.params.rest_length or 1.0
				local damping = c.params.damping or 1.0
				local pos1 = obj1:get_pos()
				local pos2 = type(obj2) == "table" and obj2:get_pos() or obj2
				local d = vector.subtract(pos2, pos1)
				local dist = vector.length(d)
				if dist > 0.001 then
					local force_mag = k * (dist - rest_length)
					local dir = vector.divide(d, dist)
					local force = vector.multiply(dir, force_mag)
					obj1:apply_force(force)
					if type(obj2) == "table" then
						obj2:apply_force(vector.multiply(force, -1))
					end
				end
			elseif c.type == "Rope" or c.type == "Chain" then
				local max_dist = c.params.max_distance or 5.0
				local pos1 = obj1:get_pos()
				local pos2 = type(obj2) == "table" and obj2:get_pos() or obj2
				local d = vector.subtract(pos2, pos1)
				local dist = vector.length(d)
				if dist > max_dist then
					local correction = vector.multiply(vector.divide(d, dist), dist - max_dist)
					obj1:set_pos(vector.add(pos1, correction))
				end
			end
		end
	end

	-- 4. Periodically auto-save persistent objects
	self.last_autosave = self.last_autosave + dtime
	if self.last_autosave >= self.autosave_interval then
		self.last_autosave = 0
		self:save_persistent_data()
	end
end

-- Serialization & Database Persistence
function WorldObjectManager:save_persistent_data()
	local path = core.get_worldpath() .. DIR_DELIM .. "mwof_objects.dat"
	local data = {}
	for id, obj in pairs(self.objects) do
		if obj.persistent then
			table.insert(data, {
				id = obj.id,
				pos = obj.pos,
				vel = obj.vel,
				acc = obj.acc,
				mass = obj.mass,
				gravity_scale = obj.gravity_scale,
				friction = obj.friction,
				bounce = obj.bounce,
				persistent = obj.persistent,
				captured_node = obj.captured_node,
				captured_area = obj.captured_area,
				mesh = obj.mesh,
				visual = obj.visual,
				textures = obj.textures,
				visual_size = obj.visual_size,
				collision_shape = obj.collision_shape,
				metadata = obj.metadata,
			})
		end
	end
	local f = io.open(path, "w")
	if f then
		f:write(core.serialize(data))
		f:close()
	end
end

function WorldObjectManager:load_persistent_data()
	local path = core.get_worldpath() .. DIR_DELIM .. "mwof_objects.dat"
	local f = io.open(path, "r")
	if not f then return end
	local content = f:read("*all")
	f:close()
	if not content or content == "" then return end
	local data = core.deserialize(content)
	if type(data) == "table" then
		for _, d in ipairs(data) do
			local obj = WorldObject:new(d)
			self.objects[obj.id] = obj
			obj:wake()
		end
	end
end

-- Register internal visual anchor entity
core.register_entity(":mwof:object", {
	static_save = false, -- Prevent the engine from saving visual anchor entities to mapblocks (avoids ghost entity leaks)
	initial_properties = {
		hp_max = 10,
		physical = true,
		collide_with_objects = true,
		collisionbox = {-0.5, -0.5, -0.5, 0.5, 0.5, 0.5},
		visual = "sprite",
		textures = {"blank.png"},
		is_visible = true,
	},
	world_object_id = nil,

	on_step = function(self, dtime, moveresult)
		if self.world_object_id then
			local obj = WorldObjectManager.objects[self.world_object_id]
			if obj then
				-- Synchronize physical values back to the Lua abstraction
				obj.pos = self.object:get_pos()
				obj.vel = self.object:get_velocity()
				obj:trigger_event("on_move")

				-- Precise C++ collision resolver hook
				if moveresult and moveresult.collides then
					for _, info in ipairs(moveresult.collisions or {}) do
						obj:trigger_event("on_collision", info)
					end
				end
			end
		end
	end,

	on_punch = function(self, hitter, time_from_last_punch, tool_capabilities, dir)
		if self.world_object_id then
			local obj = WorldObjectManager.objects[self.world_object_id]
			if obj then
				local damage = tool_capabilities and tool_capabilities.damage_groups and tool_capabilities.damage_groups.fleshy or 1
				obj:trigger_event("on_damage", damage, hitter)
				if obj.components["health"] then
					local h = obj.components["health"]
					if h.hp then
						h.hp = h.hp - damage
						if h.hp <= 0 then
							obj:destroy()
						end
					end
				end
			end
		end
	end,

	on_rightclick = function(self, clicker)
		if self.world_object_id then
			local obj = WorldObjectManager.objects[self.world_object_id]
			if obj then
				obj:trigger_event("on_click", clicker)
			end
		end
	end,
})

-- Component Definitions
WorldObjectManager:register_component("mesh", {
	on_init = function(self, obj)
		-- mesh specific properties
	end,
	set_mesh = function(self, obj, mesh)
		obj.mesh = mesh
		if obj.object_ref and obj.object_ref:is_valid() then
			obj.object_ref:set_properties({mesh = mesh, visual = "mesh"})
		end
	end,
	set_textures = function(self, obj, textures)
		obj.textures = textures
		if obj.object_ref and obj.object_ref:is_valid() then
			obj.object_ref:set_properties({textures = textures})
		end
	end,
	set_visual_size = function(self, obj, size)
		obj.visual_size = size
		if obj.object_ref and obj.object_ref:is_valid() then
			obj.object_ref:set_properties({visual_size = size})
		end
	end
})

WorldObjectManager:register_component("physics", {
	on_init = function(self, obj)
		obj.physics_enabled = true
	end
})

WorldObjectManager:register_component("light", {
	on_init = function(self, obj)
		self.light_level = 10
	end,
	set_light_level = function(self, obj, level)
		self.light_level = level
		if obj.object_ref and obj.object_ref:is_valid() then
			obj.object_ref:set_properties({glow = level})
		end
	end
})

WorldObjectManager:register_component("particle", {
	emit_particles = function(self, obj, particle_def)
		particle_def = particle_def or {}
		particle_def.pos = obj.pos
		core.add_particle(particle_def)
	end
})

WorldObjectManager:register_component("audio", {
	play_sound = function(self, obj, sound_name, sound_params)
		sound_params = sound_params or {}
		sound_params.pos = obj.pos
		core.sound_play(sound_name, sound_params)
	end
})

WorldObjectManager:register_component("script", {
	set_script = function(self, obj, func)
		self.update_func = func
	end,
	on_step = function(self, obj, dtime)
		if self.update_func then
			self.update_func(obj, dtime)
		end
	end
})

WorldObjectManager:register_component("animation", {
	play_animation = function(self, obj, clip, speed, blend, loop)
		if obj.object_ref and obj.object_ref:is_valid() then
			if obj.object_ref.set_animation_clip then
				obj.object_ref:set_animation_clip(clip, {x=0, y=100}, speed, blend, loop)
			else
				obj.object_ref:set_animation({x=0, y=100}, speed, blend, loop)
			end
		end
	end,
	tween_to = function(self, obj, target_pos, duration, easing, on_finish)
		self.tween = {
			start_pos = vector.new(obj.pos),
			target_pos = vector.new(target_pos),
			duration = duration,
			elapsed = 0,
			easing = easing or "linear",
			on_finish = on_finish
		}
	end,
	follow_path = function(self, obj, points, speed, loop, on_finish)
		self.path = {
			points = points,
			speed = speed,
			loop = loop,
			current_index = 1,
			on_finish = on_finish
		}
	end,
	on_step = function(self, obj, dtime)
		-- Process tweens
		if self.tween then
			local t = self.tween
			t.elapsed = t.elapsed + dtime
			local pct = math.min(1.0, t.elapsed / t.duration)
			if t.easing == "smoothstep" then
				pct = pct * pct * (3 - 2 * pct)
			end
			local next_pos = vector.add(t.start_pos, vector.multiply(vector.subtract(t.target_pos, t.start_pos), pct))
			obj:set_pos(next_pos)
			if pct >= 1.0 then
				if t.on_finish then t.on_finish(obj) end
				self.tween = nil
			end
		end

		-- Process path following
		if self.path then
			local p = self.path
			local current_target = p.points[p.current_index]
			if current_target then
				local dir = vector.subtract(current_target, obj.pos)
				local dist = vector.length(dir)
				local move_dist = p.speed * dtime
				if dist <= move_dist then
					obj:set_pos(current_target)
					p.current_index = p.current_index + 1
					if p.current_index > #p.points then
						if p.loop then
							p.current_index = 1
						else
							if p.on_finish then p.on_finish(obj) end
							self.path = nil
						end
					end
				else
					local step = vector.multiply(vector.divide(dir, dist), move_dist)
					obj:set_pos(vector.add(obj.pos, step))
				end
			end
		end
	end
})

WorldObjectManager:register_component("inventory", {
	on_init = function(self, obj)
		-- Create a detached inventory for this object
		local inv_id = "mwof:inv_" .. obj.id
		self.inventory = core.create_detached_inventory(inv_id, {
			allow_move = function(inv, from_list, from_index, to_list, to_index, count, player) return count end,
			allow_put = function(inv, listname, index, stack, player) return stack:get_count() end,
			allow_take = function(inv, listname, index, stack, player) return stack:get_count() end,
		})
		self.inventory:set_size("main", 32)
	end,
	get_inventory = function(self, obj)
		return self.inventory
	end
})

WorldObjectManager:register_component("health", {
	on_init = function(self, obj)
		self.hp = 100
		self.max_hp = 100
	end,
	get_hp = function(self, obj) return self.hp end,
	set_hp = function(self, obj, hp) self.hp = hp end,
	damage = function(self, obj, amount, reason)
		self.hp = self.hp - amount
		obj:trigger_event("on_damage", amount, reason)
		if self.hp <= 0 then
			obj:destroy()
		end
	end
})

WorldObjectManager:register_component("camera", {
	set_camera_target = function(self, obj, player, mode)
		if player and player:is_valid() then
			player:set_camera({mode = mode or "thirdpersonback"})
		end
	end
})

WorldObjectManager:register_component("html", {
	run_html = function(self, obj, id, html)
		if htmlview and htmlview.run then
			htmlview.run(id, html)
		end
	end
})


-- Map core global shortcuts
core.create_world_object = function(def) return WorldObjectManager:create_world_object(def) end
core.create_object = function(def) return WorldObjectManager:create_object(def) end
core.object_from_node = function(pos) return WorldObjectManager:object_from_node(pos) end
core.node_to_object = function(pos) return WorldObjectManager:node_to_object(pos) end
core.object_from_area = function(minp, maxp) return WorldObjectManager:object_from_area(minp, maxp) end
core.capture_nodes = function(minp, maxp) return WorldObjectManager:capture_nodes(minp, maxp) end
core.object_from_player = function(player) return WorldObjectManager:object_from_player(player) end
core.object_from_entity = function(entity) return WorldObjectManager:object_from_entity(entity) end
core.object_from_mesh = function(mesh) return WorldObjectManager:object_from_mesh(mesh) end
core.object_from_html = function(view) return WorldObjectManager:object_from_html(view) end


-- Hooks and system events
core.register_globalstep(function(dtime)
	WorldObjectManager:step(dtime)
end)

core.register_on_mods_loaded(function()
	WorldObjectManager:load_persistent_data()
end)

core.register_on_shutdown(function()
	WorldObjectManager:save_persistent_data()
end)
