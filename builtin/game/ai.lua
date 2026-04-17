-- Goal-based AI framework for entities

core.ai = {}
local ai = core.ai

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

local function update_entity_ai(obj, dtime)
	local state = entity_ai_states[obj]
	if not state or not state.goals or #state.goals == 0 then
		return
	end

	-- Select the best goal
	local best_goal = nil
	local best_priority = -1

	for _, goal in ipairs(state.goals) do
		local active = false
		local cond = goal.condition
		if type(cond) == "function" then
			active = cond(obj)
		elseif type(cond) == "string" then
			-- Simple string condition evaluation if obj has the method
			if type(obj[cond]) == "function" then
				active = obj[cond](obj)
			end
		elseif cond == nil then
			active = true
		end

		if active and goal.priority > best_priority then
			best_priority = goal.priority
			best_goal = goal
		end
	end

	if best_goal then
		if state.current_goal ~= best_goal then
			state.current_goal = best_goal
			state.goal_start_time = core.get_gametime()
		end

		local action = ai.registered_goals[best_goal.type]
		if action then
			local result = action(obj, best_goal, dtime)
			if result == true then -- Goal completed
				if type(best_goal.reward) == "function" then
					best_goal.reward(obj)
				end
				state.current_goal = nil
			elseif result == false then -- Goal failed
				if type(best_goal.penalty) == "function" then
					best_goal.penalty(obj)
				end
				state.current_goal = nil
			end
		end
	else
		state.current_goal = nil
	end

	ai.process_node_penalties(obj)
end

core.register_globalstep(function(dtime)
	-- Process players
	for _, player in ipairs(core.get_connected_players()) do
		ai.process_node_penalties(player)
	end

	-- Process entities with AI
	for obj, state in pairs(entity_ai_states) do
		if obj:is_valid() then
			update_entity_ai(obj, dtime)
		else
			entity_ai_states[obj] = nil
		end
	end
end)

-- Metatable patching for ObjectRef
local function patch_object_metatable(mt)
	if not mt then return end

	mt.set_goals = function(self, goals)
		if goals then
			entity_ai_states[self] = { goals = goals }
		else
			entity_ai_states[self] = nil
		end
	end

	mt.get_goals = function(self)
		return entity_ai_states[self] and entity_ai_states[self].goals
	end

	-- Override findpath to support the opt-in penalties flag
	local old_findpath = mt.findpath
	mt.findpath = function(self, targetpos, params)
		params = params or {}
		if params.penalties == true then
			local p_table = {}
			for name, def in pairs(ai.node_penalties) do
				p_table[name] = def.penalty
			end
			params.penalties = p_table
		end
		return old_findpath(self, targetpos, params)
	end

	-- Provide stubs for buff/status if not present, to match requested API examples
	if not mt.add_buff then
		mt.add_buff = function(self, name, duration)
			-- Mods should override this with real logic
		end
	end
	if not mt.add_status then
		mt.add_status = function(self, name, duration)
			-- Mods should override this with real logic
		end
	end
end

-- Patch the ObjectRef metatable
local ObjectRef_mt = debug.getregistry().ObjectRef
if ObjectRef_mt then
	patch_object_metatable(ObjectRef_mt)
end
