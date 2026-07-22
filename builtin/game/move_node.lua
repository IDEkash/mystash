local facedir_to_euler = {
	{y = 0, x = 0, z = 0},
	{y = -math.pi/2, x = 0, z = 0},
	{y = math.pi, x = 0, z = 0},
	{y = math.pi/2, x = 0, z = 0},
	{y = math.pi/2, x = -math.pi/2, z = math.pi/2},
	{y = math.pi/2, x = math.pi, z = math.pi/2},
	{y = math.pi/2, x = math.pi/2, z = math.pi/2},
	{y = math.pi/2, x = 0, z = math.pi/2},
	{y = -math.pi/2, x = math.pi/2, z = math.pi/2},
	{y = -math.pi/2, x = 0, z = math.pi/2},
	{y = -math.pi/2, x = -math.pi/2, z = math.pi/2},
	{y = -math.pi/2, x = math.pi, z = math.pi/2},
	{y = 0, x = 0, z = math.pi/2},
	{y = 0, x = -math.pi/2, z = math.pi/2},
	{y = 0, x = math.pi, z = math.pi/2},
	{y = 0, x = math.pi/2, z = math.pi/2},
	{y = math.pi, x = math.pi, z = math.pi/2},
	{y = math.pi, x = math.pi/2, z = math.pi/2},
	{y = math.pi, x = 0, z = math.pi/2},
	{y = math.pi, x = -math.pi/2, z = math.pi/2},
	{y = math.pi, x = math.pi, z = 0},
	{y = -math.pi/2, x = math.pi, z = 0},
	{y = 0, x = math.pi, z = 0},
	{y = math.pi/2, x = math.pi, z = 0}
}

local function lerp(a, b, t)
	return a + (b - a) * t
end

local function lerp_angle(a, b, t)
	local diff = (b - a + math.pi) % (math.pi * 2) - math.pi
	return a + diff * t
end

local function rotate_vector(v, rot)
	local cx, sx = math.cos(rot.x), math.sin(rot.x)
	local cy, sy = math.cos(rot.y), math.sin(rot.y)
	local cz, sz = math.cos(rot.z), math.sin(rot.z)

	-- 1. Rotate around Y (Yaw)
	local x = v.x * cy + v.z * sy
	local y = v.y
	local z = -v.x * sy + v.z * cy

	-- 2. Rotate around X (Pitch)
	local dy = y * cx - z * sx
	local dz = y * sx + z * cx
	y = dy
	z = dz

	-- 3. Rotate around Z (Roll)
	local dx = x * cz - y * sz
	dy = x * sz + y * cz
	x = dx
	y = dy

	return vector.new(x, y, z)
end

local function get_node_initial_rotation(node)
	local def = core.registered_nodes[node.name]
	if not def then return {x=0, y=0, z=0} end
	if def.paramtype2 == "facedir" then
		local fdir = (node.param2 or 0) % 32 % 24
		local euler = facedir_to_euler[fdir + 1]
		if euler then
			return {x = euler.x, y = euler.y, z = euler.z}
		end
	elseif def.paramtype2 == "4dir" then
		local fdir = (node.param2 or 0) % 4
		local euler = facedir_to_euler[fdir + 1]
		if euler then
			return {x = euler.x, y = euler.y, z = euler.z}
		end
	end
	return {x=0, y=0, z=0}
end

local function find_closest_facedir(euler_rot, paramtype2)
	local best_fdir = 0
	local min_diff = math.huge
	local limit = 24
	if paramtype2 == "4dir" then
		limit = 4
	end
	for fdir = 0, limit - 1 do
		local ref = facedir_to_euler[fdir + 1]
		if ref then
			local dx = math.abs((euler_rot.x - ref.x + math.pi) % (math.pi * 2) - math.pi)
			local dy = math.abs((euler_rot.y - ref.y + math.pi) % (math.pi * 2) - math.pi)
			local dz = math.abs((euler_rot.z - ref.z + math.pi) % (math.pi * 2) - math.pi)
			local diff = dx + dy + dz
			if diff < min_diff then
				min_diff = diff
				best_fdir = fdir
			end
		end
	end
	return best_fdir
end

local function get_eased_time(t, easing)
	if easing == "smoothstep" then
		return t * t * (3 - 2 * t)
	elseif easing == "ease_in" then
		return t * t
	elseif easing == "ease_out" then
		return t * (2 - t)
	else -- "linear"
		return t
	end
end

-- Register moving node entity
core.register_entity(":__builtin:moving_node", {
	initial_properties = {
		visual = "node",
		physical = false,
		collide_with_objects = false,
		is_visible = true,
		interpolate_position = true, -- Smooth client-side interpolation of physical entities
		static_save = true,
	},

	remove_all = function(self)
		if self.object then
			local children = self.object:get_children() or {}
			for _, child in ipairs(children) do
				child:remove()
			end
			self.object:remove()
		end
	end,

	on_punch = function(self, puncher, time_from_last_punch, tool_capabilities, dir)
		if not puncher or not puncher:is_player() then return end
		local player_name = puncher:get_player_name()

		-- Identify node info
		local node_info = self.node_info
		if not node_info and self.is_master and self.nodes and #self.nodes == 1 then
			node_info = self.nodes[1]
		end

		if not node_info then return end

		-- Check if player can dig this node
		local pos = self.object:get_pos()
		if not pos then return end
		if core.is_protected(pos, player_name) then
			core.record_protection_violation(pos, player_name)
			return
		end

		local node = node_info.node
		local def = node_info.def or core.registered_nodes[node.name]
		if not def then return end

		-- Simulate realistic block digging time and tools
		local tool = puncher:get_wielded_item()
		local dig_params = core.get_dig_params(def.groups, tool_capabilities)
		if not dig_params or not dig_params.diggable then
			-- Node is not diggable with this tool
			return
		end

		-- Play punch/crack sound and particles
		if def.sounds and def.sounds.dug then
			core.sound_play(def.sounds.dug, {pos = pos, gain = 0.5}, true)
		end

		self.dig_progress = (self.dig_progress or 0) + (time_from_last_punch or 0.2)
		self.last_punch_time = core.get_gametime()

		-- Visually crack the block
		local ratio = math.floor((self.dig_progress / dig_params.time) * 10)
		if ratio > 0 and ratio < 10 then
			self.object:set_properties({
				damage_texture_modifier = "^[crack:10:" .. ratio
			})
		end

		if self.dig_progress >= dig_params.time then
			-- Play destruction sound
			if def.sounds and def.sounds.dug then
				core.sound_play(def.sounds.dug, {pos = pos}, true)
			end

			-- Handle drops
			local drops = core.get_node_drops(node, tool:get_name())
			for _, drop in ipairs(drops) do
				core.add_item(pos, drop)
			end

			-- Wear tool
			if not core.settings:get_bool("creative_mode") then
				tool:add_wear(dig_params.wear)
				puncher:set_wielded_item(tool)
			end

			-- Remove entire structure
			if self.is_child then
				local parent = self.object:get_attach()
				if parent then
					local parent_ent = parent:get_luaentity()
					if parent_ent and parent_ent.remove_all then
						parent_ent:remove_all()
					else
						parent:remove()
					end
				end
				self.object:remove()
			elseif self.is_master then
				self:remove_all()
			else
				self.object:remove()
			end
		end
	end,

	on_rightclick = function(self, clicker)
		if not clicker or not clicker:is_player() then return end
		local node_info = self.node_info
		if not node_info and self.is_master and self.nodes and #self.nodes == 1 then
			node_info = self.nodes[1]
		end

		if not node_info then return end

		local def = node_info.def or core.registered_nodes[node_info.node.name]
		if def and def.on_rightclick then
			def.on_rightclick(self.object:get_pos(), node_info.node, clicker)
		end
	end,

	on_step = function(self, dtime)
		-- Reset dig progress if player stopped punching
		if self.last_punch_time and core.get_gametime() - self.last_punch_time > 1.5 then
			self.dig_progress = 0
			self.last_punch_time = nil
			self.object:set_properties({
				damage_texture_modifier = ""
			})
		end
	end,

	on_activate = function(self, staticdata)
		self.object:set_armor_groups({immortal = 1})
	end,
})

-- Register moving node master entity (clean, invisible placeholder)
core.register_entity(":__builtin:moving_node_master", {
	initial_properties = {
		visual = "sprite",
		visual_size = {x=0, y=0},
		physical = false,
		collide_with_objects = false,
		is_visible = false,
		static_save = true,
	},

	on_activate = function(self, staticdata)
		self.object:set_armor_groups({immortal = 1})
	end,
})

local active_moves = {}
local attached_passengers = {} -- { [playerName/objectId] = { master = obj, offset = vec } }

local MoveHandle = {}
MoveHandle.__index = MoveHandle

function MoveHandle:new(pos, opts)
	local obj = setmetatable({}, MoveHandle)
	obj.opts = opts or {}

	if pos.x and pos.y and pos.z then
		obj.min_pos = vector.new(pos)
		obj.max_pos = vector.new(pos)
		obj.pivot = vector.new(pos)
	elseif pos.min and pos.max then
		obj.min_pos = vector.new(pos.min)
		obj.max_pos = vector.new(pos.max)
		obj.pivot = vector.new(pos.min)
	elseif pos[1] and pos[2] then
		obj.min_pos = vector.new(pos[1])
		obj.max_pos = vector.new(pos[2])
		obj.pivot = vector.new(pos[1])
	else
		error("core.move_node: invalid position/region format")
	end

	return obj
end

function MoveHandle:start()
	local min_pos = self.min_pos
	local max_pos = self.max_pos
	local pivot = self.pivot

	self.nodes = {}
	self.entities = {}
	self.entity_ids = {}

	local is_movable_group = false
	local placeholder = self.opts.placeholder or "air"

	-- Calculate optimized single bounding box of the platform
	self.platform_min = { x = min_pos.x - 0.5, z = min_pos.z - 0.5 }
	self.platform_max = { x = max_pos.x + 0.5, z = max_pos.z + 0.5 }

	-- 1. Read and replace nodes from voxel grid
	for x = min_pos.x, max_pos.x do
		for y = min_pos.y, max_pos.y do
			for z = min_pos.z, max_pos.z do
				local curr_pos = vector.new(x, y, z)
				local node = core.get_node(curr_pos)
				if node.name ~= "air" and node.name ~= "ignore" then
					local def = core.registered_nodes[node.name]
					if def then
						if core.get_item_group(node.name, "movable") > 0 then
							is_movable_group = true
						end
						local meta = core.get_meta(curr_pos):to_table()

						local colbox = {-0.5, -0.5, -0.5, 0.5, 0.5, 0.5}
						if def.collision_box and def.collision_box.fixed then
							local fixed = def.collision_box.fixed
							if type(fixed[1]) == "table" then fixed = fixed[1] end
							if type(fixed) == "table" and #fixed >= 6 then colbox = fixed end
						elseif def.node_box and def.node_box.fixed then
							local fixed = def.node_box.fixed
							if type(fixed[1]) == "table" then fixed = fixed[1] end
							if type(fixed) == "table" and #fixed >= 6 then colbox = fixed end
						end

						table.insert(self.nodes, {
							offset = vector.subtract(curr_pos, pivot),
							node = node,
							meta = meta,
							glow = def.light_source or 0,
							colbox = colbox,
							def = def,
						})
						core.set_node(curr_pos, {name = placeholder})
						core.log("action", ("Moving platform start: %s replaced with placeholder at %s"):format(node.name, core.pos_to_string(curr_pos)))
					end
				end
			end
		end
	end

	if is_movable_group then
		if self.opts.collide == nil then self.opts.collide = true end
		if self.opts.easing == nil then self.opts.easing = "smoothstep" end
	end
	if self.opts.collide == nil then self.opts.collide = true end

	-- 2. Spawn entities
	local start_pos = pivot
	local start_rot = {x=0, y=0, z=0}
	if #self.nodes > 0 then
		start_rot = get_node_initial_rotation(self.nodes[1].node)
	end

	if #self.nodes == 1 and vector.equals(self.nodes[1].offset, vector.zero()) then
		local node_info = self.nodes[1]
		local obj = core.add_entity(start_pos, "__builtin:moving_node")
		if obj then
			obj:set_properties({
				node = node_info.node,
				physical = self.opts.collide,
				collide_with_objects = self.opts.collide,
				collisionbox = node_info.colbox,
				glow = node_info.glow,
			})
			obj:set_rotation(start_rot)
			self.master_entity = obj
			table.insert(self.entities, obj)
			self.entity_ids[obj:get_luaentity()] = true
		end
	else
		-- Spawn invisible master coordinator entity
		local master_obj = core.add_entity(start_pos, "__builtin:moving_node_master")
		if master_obj then
			master_obj:set_rotation(start_rot)
			self.master_entity = master_obj
			table.insert(self.entities, master_obj)
			self.entity_ids[master_obj:get_luaentity()] = true

			for _, node_info in ipairs(self.nodes) do
				local child_pos = vector.add(start_pos, node_info.offset)
				local child_obj = core.add_entity(child_pos, "__builtin:moving_node")
				if child_obj then
					child_obj:set_properties({
						node = node_info.node,
						physical = self.opts.collide,
						collide_with_objects = self.opts.collide,
						collisionbox = node_info.colbox,
						glow = node_info.glow,
					})
					child_obj:set_rotation(get_node_initial_rotation(node_info.node))
					child_obj:set_attach(master_obj, "", vector.multiply(node_info.offset, 10), {x=0, y=0, z=0})

					table.insert(self.entities, child_obj)
					self.entity_ids[child_obj:get_luaentity()] = true
				end
			end
		end
	end

	self.start_rot = start_rot
	self.last_pos = start_pos
	self.last_rot = start_rot

	-- Initialize waypoint legs
	self.legs = {}
	local waypoints = self.opts.waypoints
	if not waypoints then
		local duration = self.opts.time or (self.opts.speed and (vector.distance(start_pos, self.opts.to) / self.opts.speed)) or 1.0
		waypoints = {{pos = self.opts.to, time = duration}}
	end

	local last_p = start_pos
	local last_r = start_rot
	local cumulative_time = 0
	for _, wp in ipairs(waypoints) do
		local duration = wp.time or (self.opts.speed and (vector.distance(last_p, wp.pos) / self.opts.speed)) or 1.0
		local target_pos = wp.pos
		local target_rot = wp.rotation and {x = math.rad(wp.rotation.x), y = math.rad(wp.rotation.y), z = math.rad(wp.rotation.z)} or (self.opts.rotation and {x = math.rad(self.opts.rotation.x), y = math.rad(self.opts.rotation.y), z = math.rad(self.opts.rotation.z)}) or last_r
		table.insert(self.legs, {
			pos_a = last_p,
			pos_b = target_pos,
			rot_a = last_r,
			rot_b = target_rot,
			start_time = cumulative_time,
			end_time = cumulative_time + duration,
			duration = duration,
		})
		cumulative_time = cumulative_time + duration
		last_p = target_pos
		last_r = target_rot
	end

	self.total_duration = cumulative_time
	self.elapsed_time = 0
	self.pingpong_direction = 1
	self.paused = false
	self.active = true
	self.current_progress = 0

	table.insert(active_moves, self)
end

local function get_interpolation(self, t)
	local leg
	for _, l in ipairs(self.legs) do
		if t >= l.start_time and t <= l.end_time then
			leg = l
			break
		end
	end
	if not leg then
		if t < 0 then
			leg = self.legs[1]
		else
			leg = self.legs[#self.legs]
		end
	end

	local factor = 0
	if leg.duration > 0.0001 then
		factor = (t - leg.start_time) / leg.duration
	else
		factor = 1.0
	end
	if factor < 0 then factor = 0 elseif factor > 1 then factor = 1 end

	local eased_factor = get_eased_time(factor, self.opts.easing)

	local current_pos = vector.new(
		lerp(leg.pos_a.x, leg.pos_b.x, eased_factor),
		lerp(leg.pos_a.y, leg.pos_b.y, eased_factor),
		lerp(leg.pos_a.z, leg.pos_b.z, eased_factor)
	)

	local current_rot = {
		x = lerp_angle(leg.rot_a.x, leg.rot_b.x, eased_factor),
		y = lerp_angle(leg.rot_a.y, leg.rot_b.y, eased_factor),
		z = lerp_angle(leg.rot_a.z, leg.rot_b.z, eased_factor)
	}

	return current_pos, current_rot, factor
end

function MoveHandle:step(dtime)
	if self.paused or not self.active then return end

	if not self.master_entity or not self.master_entity:get_pos() then
		self:stop()
		return
	end

	local next_time = self.elapsed_time + dtime * self.pingpong_direction
	local finished = false

	if self.pingpong_direction == 1 then
		if next_time >= self.total_duration then
			if self.opts.loop == "pingpong" then
				next_time = self.total_duration
				self.pingpong_direction = -1
			elseif self.opts.loop then
				next_time = next_time % self.total_duration
			else
				next_time = self.total_duration
				finished = true
			end
		end
	else
		if next_time <= 0 then
			next_time = 0
			self.pingpong_direction = 1
		end
	end

	self.elapsed_time = next_time

	local current_pos, current_rot, leg_progress = get_interpolation(self, next_time)
	self.current_progress = leg_progress

	local prev_pos = self.master_entity:get_pos()
	self.master_entity:set_pos(current_pos)
	self.master_entity:set_rotation(current_rot)

	local delta = vector.subtract(current_pos, prev_pos)

	-- 100% Smooth attachment-based player rider support
	if self.opts.collide and vector.length(delta) > 0.0001 then
		local players = core.get_connected_players()
		for _, player in ipairs(players) do
			local name = player:get_player_name()
			local attach_info = attached_passengers[name]

			if attach_info then
				-- Detach ONLY if they press Jump to jump off
				local ctrl = player:get_player_control()
				if ctrl.jump then
					player:set_detach()
					attached_passengers[name] = nil
					-- Provide a natural upward jump velocity boost
					player:add_velocity({x=0, y=8, z=0})
				end
			else
				-- Detect if standing on top of platform bounding box
				local ppos = player:get_pos()
				local box_min = self.platform_min
				local box_max = self.platform_max

				-- Apply translation delta of platform to the bounding box checks
				local current_min_x = box_min.x + (current_pos.x - self.pivot.x)
				local current_max_x = box_max.x + (current_pos.x - self.pivot.x)
				local current_min_z = box_min.z + (current_pos.z - self.pivot.z)
				local current_max_z = box_max.z + (current_pos.z - self.pivot.z)
				local platform_y = current_pos.y

				if ppos.x >= current_min_x and ppos.x <= current_max_x and
				   ppos.z >= current_min_z and ppos.z <= current_max_z and
				   ppos.y >= platform_y + 0.4 and ppos.y <= platform_y + 1.2 then

					-- Attach player smoothly to master entity!
					local offset = vector.subtract(ppos, current_pos)
					player:set_attach(self.master_entity, "", vector.multiply(offset, 10), {x=0, y=0, z=0})
					attached_passengers[name] = { master = self.master_entity, offset = offset }
				end
			end
		end

		-- Shift other non-player entities in radius
		local objects = core.get_objects_inside_radius(prev_pos, 15)
		for _, obj in ipairs(objects) do
			if obj:get_luaentity() and not self.entity_ids[obj:get_luaentity()] and not obj:is_player() then
				local opos = obj:get_pos()
				if opos then
					local box_min = self.platform_min
					local box_max = self.platform_max
					local current_min_x = box_min.x + (current_pos.x - self.pivot.x)
					local current_max_x = box_max.x + (current_pos.x - self.pivot.x)
					local current_min_z = box_min.z + (current_pos.z - self.pivot.z)
					local current_max_z = box_max.z + (current_pos.z - self.pivot.z)
					local platform_y = current_pos.y

					if opos.x >= current_min_x and opos.x <= current_max_x and
					   opos.z >= current_min_z and opos.z <= current_max_z and
					   opos.y >= platform_y + 0.4 and opos.y <= platform_y + 1.2 then

						obj:set_pos(vector.add(opos, delta))
					end
				end
			end
		end
	end

	if self.opts.on_step then
		self.opts.on_step(self, dtime)
	end

	if finished then
		self:complete()
	end
end

function MoveHandle:complete()
	self.active = false
	local final_pos = self.legs[#self.legs].pos_b
	local final_rot = self.legs[#self.legs].rot_b

	-- Detach passengers
	local players = core.get_connected_players()
	for _, player in ipairs(players) do
		local name = player:get_player_name()
		if attached_passengers[name] and attached_passengers[name].master == self.master_entity then
			player:set_detach()
			attached_passengers[name] = nil
		end
	end

	-- Write real nodes back to the voxel grid (Requirement 3 complete)
	for _, node_info in ipairs(self.nodes) do
		local rot_offset = rotate_vector(node_info.offset, final_rot)
		local rounded_pos = vector.round(vector.add(final_pos, rot_offset))

		local placing_node = table.copy(node_info.node)
		local def = node_info.def
		if def and (def.paramtype2 == "facedir" or def.paramtype2 == "4dir") then
			placing_node.param2 = find_closest_facedir(final_rot, def.paramtype2)
		end

		core.set_node(rounded_pos, placing_node)
		if node_info.meta then
			core.get_meta(rounded_pos):from_table(node_info.meta)
		end
		core.log("action", ("Moving platform complete: %s placed at %s"):format(placing_node.name, core.pos_to_string(rounded_pos)))
	end

	for _, ent in ipairs(self.entities) do
		if ent:get_pos() then
			ent:remove()
		end
	end

	if self.opts.on_complete then
		self.opts.on_complete(self)
	end

	for i, h in ipairs(active_moves) do
		if h == self then
			table.remove(active_moves, i)
			break
		end
	end
end

function MoveHandle:stop()
	if not self.active then return end
	self.active = false

	local current_pos = self.pivot
	local current_rot = self.start_rot

	if self.master_entity and self.master_entity:get_pos() then
		current_pos = self.master_entity:get_pos()
		current_rot = self.master_entity:get_rotation() or {x=0, y=0, z=0}
	end

	-- Detach passengers
	local players = core.get_connected_players()
	for _, player in ipairs(players) do
		local name = player:get_player_name()
		if attached_passengers[name] and attached_passengers[name].master == self.master_entity then
			player:set_detach()
			attached_passengers[name] = nil
		end
	end

	-- Write real nodes back to the voxel grid (Requirement 3 stop)
	for _, node_info in ipairs(self.nodes) do
		local rot_offset = rotate_vector(node_info.offset, current_rot)
		local rounded_pos = vector.round(vector.add(current_pos, rot_offset))

		local placing_node = table.copy(node_info.node)
		local def = node_info.def
		if def and (def.paramtype2 == "facedir" or def.paramtype2 == "4dir") then
			placing_node.param2 = find_closest_facedir(current_rot, def.paramtype2)
		end

		core.set_node(rounded_pos, placing_node)
		if node_info.meta then
			core.get_meta(rounded_pos):from_table(node_info.meta)
		end
		core.log("action", ("Moving platform stopped: %s placed at %s"):format(placing_node.name, core.pos_to_string(rounded_pos)))
	end

	for _, ent in ipairs(self.entities) do
		if ent:get_pos() then
			ent:remove()
		end
	end

	for i, h in ipairs(active_moves) do
		if h == self then
			table.remove(active_moves, i)
			break
		end
	end
end

function MoveHandle:pause()
	self.paused = true
end

function MoveHandle:resume()
	self.paused = false
end

function MoveHandle:get_progress()
	return self.current_progress
end

function MoveHandle:get_position()
	if self.master_entity and self.master_entity:get_pos() then
		return self.master_entity:get_pos()
	end
	return self.pivot
end

function core.move_node(pos, opts)
	local handle = MoveHandle:new(pos, opts)
	handle:start()
	return handle
end

core.register_globalstep(function(dtime)
	for i = #active_moves, 1, -1 do
		local handle = active_moves[i]
		if handle and handle.active then
			handle:step(dtime)
		end
	end
end)
