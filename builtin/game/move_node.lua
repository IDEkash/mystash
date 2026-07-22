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
		static_save = true, -- Persist smooth blocks across restarts!
	},

	on_activate = function(self, staticdata)
		self.object:set_armor_groups({immortal = 1})
		if staticdata and staticdata ~= "" then
			local data = core.deserialize(staticdata)
			if data then
				self.is_master = data.is_master
				self.nodes = data.nodes
				self.collision = data.collision
				self.is_child = data.is_child
				self.node_info = data.node_info

				if self.is_child then
					self.object:set_properties({
						node = self.node_info.node,
						physical = self.collision,
						collide_with_objects = self.collision,
						collisionbox = self.node_info.colbox,
						glow = self.node_info.glow,
						static_save = false,
					})
				elseif self.is_master then
					if #self.nodes == 1 and vector.equals(self.nodes[1].offset, vector.zero()) then
						local node_info = self.nodes[1]
						self.object:set_properties({
							node = node_info.node,
							physical = self.collision,
							collide_with_objects = self.collision,
							collisionbox = node_info.colbox,
							glow = node_info.glow,
						})
					else
						self.object:set_properties({
							is_visible = false,
							physical = false,
							collide_with_objects = false,
						})
						core.after(0.1, function()
							if not self.object or not self.object:get_pos() then return end
							for _, node_info in ipairs(self.nodes) do
								local child_pos = vector.add(self.object:get_pos(), node_info.offset)
								local child_obj = core.add_entity(child_pos, "__builtin:moving_node")
								if child_obj then
									child_obj:set_properties({
										node = node_info.node,
										physical = self.collision,
										collide_with_objects = self.collision,
										collisionbox = node_info.colbox,
										glow = node_info.glow,
										static_save = false,
									})
									child_obj:get_luaentity().is_child = true
									child_obj:get_luaentity().node_info = node_info
									child_obj:get_luaentity().collision = self.collision
									child_obj:set_attach(self.object, "", vector.multiply(node_info.offset, 10), {x=0, y=0, z=0})
								end
							end
						end)
					end
				end
			end
		end
	end,

	get_staticdata = function(self)
		local data = {
			is_master = self.is_master,
			nodes = self.nodes,
			collision = self.collision,
			is_child = self.is_child,
			node_info = self.node_info,
		}
		return core.serialize(data)
	end,
})

local active_moves = {}

local function find_moving_node_entity(pos)
	local objects = core.get_objects_inside_radius(pos, 0.1)
	for _, obj in ipairs(objects) do
		local luaent = obj:get_luaentity()
		if luaent and luaent.name == "__builtin:moving_node" and luaent.is_master then
			return obj
		end
	end
	return nil
end

local MoveHandle = {}
MoveHandle.__index = MoveHandle

function MoveHandle:new(pos, opts)
	local obj = setmetatable({}, MoveHandle)
	obj.opts = opts or {}

	if type(pos) == "userdata" or (type(pos) == "table" and pos.get_pos) then
		obj.existing_master = pos
		obj.pivot = pos:get_pos()
		obj.min_pos = obj.pivot
		obj.max_pos = obj.pivot
	elseif pos.x and pos.y and pos.z then
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
		error("core.move_node: invalid position/region/ObjectRef format")
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

	local existing_master = self.existing_master
	if not existing_master and self.min_pos and self.max_pos and vector.equals(self.min_pos, self.max_pos) then
		existing_master = find_moving_node_entity(self.min_pos)
	end

	local is_movable_group = false
	local start_pos = pivot
	local start_rot = {x=0, y=0, z=0}

	if existing_master then
		self.master_entity = existing_master
		local luaent = existing_master:get_luaentity()
		if luaent then
			self.nodes = luaent.nodes or {}
			table.insert(self.entities, existing_master)
			self.entity_ids[luaent] = true

			local children = existing_master:get_children() or {}
			for _, child in ipairs(children) do
				table.insert(self.entities, child)
				local child_lua = child:get_luaentity()
				if child_lua then
					self.entity_ids[child_lua] = true
				end
			end
		end
		start_pos = existing_master:get_pos()
		start_rot = existing_master:get_rotation() or {x=0, y=0, z=0}
	else
		-- Read and replace nodes from voxel grid
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
							})
							core.remove_node(curr_pos)
							core.log("action", ("Moving platform start: %s removed at %s"):format(node.name, core.pos_to_string(curr_pos)))
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

				local luaent = obj:get_luaentity()
				if luaent then
					luaent.is_master = true
					luaent.nodes = self.nodes
					luaent.collision = self.opts.collide
				end

				self.master_entity = obj
				table.insert(self.entities, obj)
				self.entity_ids[luaent] = true
			end
		else
			local master_obj = core.add_entity(start_pos, "__builtin:moving_node")
			if master_obj then
				master_obj:set_properties({
					is_visible = false,
					physical = false,
					collide_with_objects = false,
				})
				master_obj:set_rotation(start_rot)

				local luaent = master_obj:get_luaentity()
				if luaent then
					luaent.is_master = true
					luaent.nodes = self.nodes
					luaent.collision = self.opts.collide
				end

				self.master_entity = master_obj
				table.insert(self.entities, master_obj)
				self.entity_ids[luaent] = true

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
							static_save = false,
						})
						child_obj:set_rotation(get_node_initial_rotation(node_info.node))
						child_obj:set_attach(master_obj, "", vector.multiply(node_info.offset, 10), {x=0, y=0, z=0})

						local child_lua = child_obj:get_luaentity()
						if child_lua then
							child_lua.is_child = true
							child_lua.node_info = node_info
							child_lua.collision = self.opts.collide
						end

						table.insert(self.entities, child_obj)
						self.entity_ids[child_lua] = true
					end
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

	-- Rider/Passenger support
	if self.opts.collide and vector.length(delta) > 0.0001 then
		local players = core.get_connected_players()
		for _, player in ipairs(players) do
			local ppos = player:get_pos()
			local on_platform = false

			for _, node_info in ipairs(self.nodes) do
				local rot_offset = rotate_vector(node_info.offset, current_rot)
				local node_world_pos = vector.add(prev_pos, rot_offset)
				local diff_x = math.abs(ppos.x - node_world_pos.x)
				local diff_z = math.abs(ppos.z - node_world_pos.z)
				local diff_y = ppos.y - node_world_pos.y

				-- Player is on top of this node
				if diff_x < 0.8 and diff_z < 0.8 and diff_y >= 0.4 and diff_y <= 1.2 then
					on_platform = true
					break
				end
			end

			if on_platform then
				player:set_pos(vector.add(ppos, delta))
			end
		end

		local objects = core.get_objects_inside_radius(prev_pos, 15)
		for _, obj in ipairs(objects) do
			if obj:get_luaentity() and not self.entity_ids[obj:get_luaentity()] and not obj:is_player() then
				local opos = obj:get_pos()
				if opos then
					local on_platform = false
					for _, node_info in ipairs(self.nodes) do
						local rot_offset = rotate_vector(node_info.offset, current_rot)
						local node_world_pos = vector.add(prev_pos, rot_offset)
						local diff_x = math.abs(opos.x - node_world_pos.x)
						local diff_z = math.abs(opos.z - node_world_pos.z)
						local diff_y = opos.y - node_world_pos.y

						if diff_x < 0.8 and diff_z < 0.8 and diff_y >= 0.4 and diff_y <= 1.2 then
							on_platform = true
							break
						end
					end

					if on_platform then
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

	-- Keep blocks as floating-point entities, DO NOT rounded-write back to voxel grid!
	if self.master_entity and self.master_entity:get_pos() then
		self.master_entity:set_pos(final_pos)
		self.master_entity:set_rotation(final_rot)

		local luaent = self.master_entity:get_luaentity()
		if luaent then
			luaent.is_master = true
			luaent.nodes = self.nodes
			luaent.collision = self.opts.collide
		end

		core.log("action", ("Moving platform complete: kept as smooth entity at %s"):format(core.pos_to_string(final_pos)))
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

	if self.master_entity and self.master_entity:get_pos() then
		local current_pos = self.master_entity:get_pos()
		local current_rot = self.master_entity:get_rotation() or {x=0, y=0, z=0}

		self.master_entity:set_pos(current_pos)
		self.master_entity:set_rotation(current_rot)

		local luaent = self.master_entity:get_luaentity()
		if luaent then
			luaent.is_master = true
			luaent.nodes = self.nodes
			luaent.collision = self.opts.collide
		end

		core.log("action", ("Moving platform stopped: kept as smooth entity at %s"):format(core.pos_to_string(current_pos)))
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
