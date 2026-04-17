-- Goal-based AI framework for entities

core.ai = {}
local ai = core.ai

ai.debug = false
ai.registered_goals = {}
ai.node_penalties = {}

function core.register_goal(name, action)
	assert(type(name) == "string")
	assert(type(action) == "function")
	ai.registered_goals[name] = action
end

function core.registernodepenalty(name, def)
	assert(type(name) == "string")
	assert(type(def) == "table")
	ai.node_penalties[name] = def
end

-- Penalty effects management
function ai.process_node_penalties(obj)
	if not obj:is_valid() then return end
	local pos = obj:get_pos()
	if not pos then return end
	local node_pos = vector.round(pos)
	local node = core.get_node(node_pos)
	local def = ai.node_penalties[node.name]
	if def and def.effect then
		if type(def.effect) == "function" then
			def.effect(obj)
		elseif type(def.effect) == "string" then
			if obj.add_status then
				obj:add_status(def.effect, 1)
			elseif obj.add_buff then
				obj:add_buff(def.effect, 1)
			end
		end
	end
end

-- Goal Selector logic
local entity_ai_states = setmetatable({}, { __mode = "k" })

local function get_or_create_ai_state(obj)
	local state = entity_ai_states[obj]
	if not state then
		state = {
			goals = {},
			current_goal = nil,
			behavior_state = "idle",
			state_modifiers = {},
			memory = {
				last_seen_pos = nil,
				detected_targets = {},
				visible_targets = {},
			},
			traits = {
				aggression = 0.5,
				fear = 0.5,
				curiosity = 0.5,
			},
			perception = {
				sight_enabled = false,
				detection_range = 0,
				vision = { distance = 0, fov = 0 },
				los_check = false,
				detected_targets = {},
				visible_targets = {},
				last_seen_pos = {},
			},
			path_execution = {
				current_path = nil,
				target_pos = nil,
				stuck_timer = 0,
				last_pos = nil,
				repath_timer = 0,
			},
			last_eval_time = 0,
			eval_interval = 0.2,
			cooldowns = {},
			goal_start_time = 0,
			accumulated_dt = 0,
		}
		entity_ai_states[obj] = state
	end
	return state
end

local function get_entity_eye_pos(obj)
	local pos = obj:get_pos()
	if not pos then return nil end
	if obj:is_player() then
		return vector.add(pos, {x=0, y=1.625, z=0})
	end
	local props = obj:get_properties()
	local eye_height = props and props.eye_height or 1.5
	return vector.add(pos, {x=0, y=eye_height, z=0})
end

local function trigger_event(obj, event, ...)
	if not obj:is_valid() then return end
	if obj:is_player() then
		if obj[event] then obj[event](obj, ...) end
		return
	end
	local ent = obj:get_luaentity()
	if ent and ent[event] then
		ent[event](ent, ...)
	end
end

-- Perception update logic
function ai.update_perception(obj, state, dtime)
	local p = state.perception
	if not p.sight_enabled then return end

	local my_pos = obj:get_pos()
	if not my_pos then return end
	local my_eye_pos = get_entity_eye_pos(obj)
	local my_dir = obj:get_look_dir()

	local range = p.detection_range or 0
	local vision = p.vision or { distance = 0, fov = 0 }
	local max_dist = math.max(range, vision.distance)

	if max_dist <= 0 then return end

	local objects = core.get_objects_inside_radius(my_pos, max_dist)
	local current_detected = {}
	local current_visible = {}

	for _, target in ipairs(objects) do
		if target ~= obj and target:get_pos() then
			local t_pos = target:get_pos()
			local dist = vector.distance(my_pos, t_pos)

			-- Range detection (e.g. hearing/smell)
			local detected = dist <= range

			-- Vision detection (FOV + LOS)
			local visible = false
			if dist <= vision.distance then
				local to_target = vector.direction(my_pos, t_pos)
				local dot = vector.dot(my_dir, to_target)
				local angle = math.deg(math.acos(math.max(-1, math.min(1, dot))))
				if angle <= (vision.fov / 2) then
					if not p.los_check or core.line_of_sight(my_eye_pos, get_entity_eye_pos(target)) then
						visible = true
					end
				end
			end

			if detected then
				current_detected[target] = true
				if not p.detected_targets[target] then
					trigger_event(obj, "on_detect", target)
				end
			end

			if visible then
				current_visible[target] = true
				p.last_seen_pos[target] = t_pos
				state.memory.last_seen_pos = t_pos
				if not p.visible_targets[target] then
					trigger_event(obj, "on_see", target)
				end
			end
		end
	end

	-- Handle lost targets
	for target, _ in pairs(p.detected_targets) do
		if not current_detected[target] and not current_visible[target] then
			trigger_event(obj, "on_lost", target)
		end
	end

	p.detected_targets = current_detected
	p.visible_targets = current_visible

	-- Sync to memory for modder convenience
	state.memory.detected_targets = current_detected
	state.memory.visible_targets = current_visible
end

local function draw_debug(obj, state)
	if not ai.debug then return end
	local pos = obj:get_pos()
	if not pos then return end

	-- Visualization
	local p = state.perception
	if p.sight_enabled then
		core.add_particle({
			pos = vector.add(pos, {x=0, y=2.2, z=0}),
			velocity = {x=0, y=0.1, z=0},
			expirationtime = 0.5,
			size = 1,
			texture = "heart.png^[colorize:#00FF00:127",
			glow = 10,
		})
	end

	if state.current_goal then
		-- Debug string could be sent via chat or displayed in nametag
	end
end

local function update_path_execution(obj, state, dtime)
	local pe = state.path_execution
	if not pe.target_pos then return end

	local my_pos = obj:get_pos()
	if not my_pos then return end

	-- Stuck detection
	if pe.last_pos and vector.distance(my_pos, pe.last_pos) < 0.1 then
		pe.stuck_timer = pe.stuck_timer + dtime
	else
		pe.stuck_timer = 0
	end
	pe.last_pos = my_pos

	if pe.stuck_timer > 2.0 then
		pe.current_path = nil
		pe.stuck_timer = 0
	end

	-- Repath logic
	pe.repath_timer = pe.repath_timer + dtime
	if not pe.current_path or pe.repath_timer > 5.0 then
		pe.repath_timer = 0
		pe.current_path = obj:findpath(pe.target_pos, {max_distance=40, penalties=true})
	end

	-- Follow path
	if pe.current_path and #pe.current_path > 0 then
		local next_node = pe.current_path[1]
		if vector.distance(my_pos, next_node) < 1.2 then
			table.remove(pe.current_path, 1)
			if #pe.current_path == 0 then
				pe.target_pos = nil
				obj:set_velocity({x=0, y=0, z=0})
				return
			end
			next_node = pe.current_path[1]
		end

		local dir = vector.direction(my_pos, next_node)
		local speed = 2
		obj:set_velocity(vector.multiply(dir, speed))

		local yaw = math.atan2(dir.z, dir.x) - math.pi / 2
		obj:set_yaw(yaw)
	end
end

local function update_entity_ai(obj, dtime)
	local state = get_or_create_ai_state(obj)
	local now = core.get_gametime()

	-- 1. Updates
	ai.update_perception(obj, state, dtime)
	update_path_execution(obj, state, dtime)

	-- 2. Interrupts
	if state.current_goal and state.current_goal.interrupt then
		if state.current_goal.interrupt(obj) then
			if state.current_goal.on_end then
				state.current_goal.on_end(obj, "interrupted")
			end
			state.current_goal = nil
		end
	end

	-- 3. Goal Selection
	if not state.current_goal or (now - state.last_eval_time >= state.eval_interval) then
		state.last_eval_time = now

		local best_goal = nil
		local best_score = -1

		for _, goal in ipairs(state.goals) do
			local in_cooldown = goal.cooldown and state.cooldowns[goal] and (now < state.cooldowns[goal])
			if not in_cooldown then
				local condition_value = 0
				local cond = goal.condition
				if type(cond) == "function" then
					local res = cond(obj)
					condition_value = type(res) == "number" and res or (res and 1 or 0)
				elseif type(cond) == "string" and type(obj[cond]) == "function" then
					local res = obj[cond](obj)
					condition_value = type(res) == "number" and res or (res and 1 or 0)
				elseif cond == nil then
					condition_value = 1
				end

				if condition_value > 0 then
					local score = (goal.priority or 1) * condition_value

					-- Multipliers
					local modifiers = state.state_modifiers[state.behavior_state]
					if modifiers and modifiers[goal.type] then
						score = score * modifiers[goal.type]
					end

					if goal.trait and state.traits[goal.trait] then
						score = score * state.traits[goal.trait]
					end

					if goal.score then
						score = score * goal.score(obj)
					end

					if score > best_score then
						best_score = score
						best_goal = goal
					end
				end
			end
		end

		if best_goal and best_goal ~= state.current_goal then
			if state.current_goal and state.current_goal.on_end then
				state.current_goal.on_end(obj, "interrupted")
			end
			state.current_goal = best_goal
			state.goal_start_time = now
			if state.current_goal.on_start then
				state.current_goal.on_start(obj)
			end
		end
	end

	-- 4. Execution
	if state.current_goal then
		local goal = state.current_goal

		if goal.duration and (now - state.goal_start_time > goal.duration) then
			if goal.on_end then goal.on_end(obj, "completed") end
			if goal.reward then goal.reward(obj) end
			if goal.cooldown then state.cooldowns[goal] = now + goal.cooldown end
			state.current_goal = nil
			return
		end

		if goal.tick_reward then goal.tick_reward(obj, dtime) end

		local action = ai.registered_goals[goal.type]
		local result
		if action then result = action(obj, goal, dtime) end
		if goal.on_tick then goal.on_tick(obj, dtime) end

		if result == true then -- Completed
			if goal.on_end then goal.on_end(obj, "completed") end
			if goal.reward then goal.reward(obj) end
			if goal.cooldown then state.cooldowns[goal] = now + goal.cooldown end
			state.current_goal = nil
		elseif result == false then -- Failed
			if goal.on_end then goal.on_end(obj, "failed") end
			if goal.penalty then goal.penalty(obj) end
			if goal.cooldown then state.cooldowns[goal] = now + goal.cooldown end
			state.current_goal = nil
		end
	end

	ai.process_node_penalties(obj)
	draw_debug(obj, state)
end

local function get_min_dist_to_player(pos)
	local min_dist = math.huge
	local players = core.get_connected_players()
	if #players == 0 then return 0 end
	for _, player in ipairs(players) do
		local ppos = player:get_pos()
		if ppos then
			min_dist = math.min(min_dist, vector.distance(pos, ppos))
		end
	end
	return min_dist
end

local staggered_counter = 0
core.register_globalstep(function(dtime)
	staggered_counter = staggered_counter + 1

	for _, player in ipairs(core.get_connected_players()) do
		ai.process_node_penalties(player)
	end

	local i = 0
	for obj, state in pairs(entity_ai_states) do
		if obj:is_valid() then
			state.accumulated_dt = state.accumulated_dt + dtime

			local pos = obj:get_pos()
			local dist = pos and get_min_dist_to_player(pos) or 0

			local skip_ticks = 3
			if dist > 100 then skip_ticks = 20
			elseif dist > 50 then skip_ticks = 10 end

			if (i + staggered_counter) % skip_ticks == 0 then
				update_entity_ai(obj, state.accumulated_dt)
				state.accumulated_dt = 0
			end
			i = i + 1
		else
			entity_ai_states[obj] = nil
		end
	end
end)

-- ObjectRef API Extension
local function patch_object_metatable(mt)
	if not mt then return end

	mt.set_state = function(self, behavior_state)
		get_or_create_ai_state(self).behavior_state = behavior_state
	end
	mt.get_state = function(self)
		local state = entity_ai_states[self]
		return state and state.behavior_state or "idle"
	end
	mt.set_state_modifiers = function(self, modifiers)
		get_or_create_ai_state(self).state_modifiers = modifiers
	end
	mt.set_goals = function(self, goals)
		get_or_create_ai_state(self).goals = goals
	end
	mt.get_goals = function(self)
		local state = entity_ai_states[self]
		return state and state.goals
	end
	mt.set_sight = function(self, enabled)
		get_or_create_ai_state(self).perception.sight_enabled = enabled
	end
	mt.has_sight = function(self)
		local state = entity_ai_states[self]
		return state and state.perception.sight_enabled
	end
	mt.set_detection_range = function(self, range)
		get_or_create_ai_state(self).perception.detection_range = range
	end
	mt.set_vision = function(self, vision_def)
		get_or_create_ai_state(self).perception.vision = vision_def
	end
	mt.set_los_check = function(self, enabled)
		get_or_create_ai_state(self).perception.los_check = enabled
	end
	mt.get_memory = function(self)
		local state = entity_ai_states[self]
		return state and state.memory
	end
	mt.get_traits = function(self)
		local state = entity_ai_states[self]
		return state and state.traits
	end
	mt.set_trait = function(self, trait, value)
		get_or_create_ai_state(self).traits[trait] = value
	end
	mt.get_perception = function(self)
		local state = entity_ai_states[self]
		return state and state.perception
	end
	mt.goto_pos = function(self, pos)
		local pe = get_or_create_ai_state(self).path_execution
		pe.target_pos = pos
		pe.current_path = nil
	end
	mt.stop_moving = function(self)
		local pe = get_or_create_ai_state(self).path_execution
		pe.target_pos = nil
		pe.current_path = nil
		self:set_velocity({x=0, y=0, z=0})
	end
	mt.can_see = function(self, target)
		local state = entity_ai_states[self]
		if not state then return false end
		if type(target) == "string" then
			for t, _ in pairs(state.perception.visible_targets) do
				if (t:is_player() and target == "player") or
				   (not t:is_player() and t:get_luaentity() and t:get_luaentity().name == target) then
					if state.perception.detected_targets[t] then return true end
				end
			end
			return false
		else
			return state.perception.visible_targets[target] == true and state.perception.detected_targets[target] == true
		end
	end

	local old_findpath = mt.findpath
	mt.findpath = function(self, targetpos, params)
		params = params or {}
		if params.penalties == true then
			local p_table = {}
			for name, def in pairs(ai.node_penalties) do p_table[name] = def.penalty end
			params.penalties = p_table
		end
		if old_findpath then return old_findpath(self, targetpos, params) end
	end

	if not mt.add_buff then mt.add_buff = function(self, name, duration) end end
	if not mt.add_status then mt.add_status = function(self, name, duration) end end

	-- Expose .memory, .traits, .perception as fields
	local methods = {}
	for k, v in pairs(mt) do methods[k] = v end
	mt.__index = function(self, key)
		if key == "memory" then return mt.get_memory(self) end
		if key == "traits" then return mt.get_traits(self) end
		if key == "perception" then return mt.get_perception(self) end
		return methods[key]
	end
end

local ObjectRef_mt = debug.getregistry().ObjectRef
if ObjectRef_mt then
	patch_object_metatable(ObjectRef_mt)
end
