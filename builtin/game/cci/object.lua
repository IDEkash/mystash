-- CCI Object Interface Implementation (Built-in - Multiplayer-Safe)
-- Manages Points, Chains, Transforms, Styling, Attributes, Events, and Hierarchy.

local Object = {}
Object.__index = Object

function cci.create_object(player_name, options)
	options = options or {}
	local session = cci.get_session(player_name)
	local obj = setmetatable({
		player_name = player_name,
		id = options.id or ("obj_" .. session.next_id),
		type = options.type or "rectangle", -- rectangle, circle, shape, textbox, custom
		points = {},
		chains = {},
		transform = {
			x = options.x or 0,
			y = options.y or 0,
			z = options.z or 0, -- depth (2.5D)
			rotation = options.rotation or 0,
			scale = options.scale or 1,
		},
		style = options.style or {},
		attributes = {
			style = {},
			action = {},
			motion = {},
		},
		events = {},
		conditions = {},
		actions = {},
		parent = nil,
		children = {},
		layer = options.layer or 0,
		safe_area = options.safe_area or false,
		visible = (options.visible ~= false),
	}, Object)

	if options.id == nil then
		session.next_id = session.next_id + 1
	end

	-- Apply initial styles if specified in options
	for k, v in pairs(options.style or {}) do
		obj:set_style(k, v)
	end

	session.objects[obj.id] = obj
	session.is_dirty = true
	return obj
end

-- Geometry: Points & Chains (Chapter 2)
function Object:add_point(x, y)
	local pt = { x = x, y = y }
	table.insert(self.points, pt)
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return #self.points
end

function Object:add_chain(...)
	local chain = {...}
	table.insert(self.chains, chain)
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return #self.chains
end

-- Styling & Custom CSS (Chapter 3)
function Object:set_style(key, value)
	self.style[key] = value
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

function Object:set_styles(tbl)
	for k, v in pairs(tbl) do
		self.style[k] = v
	end
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

-- Transform controls (Chapter 2 / Chapter 4)
function Object:set_pos(x, y, z)
	self.transform.x = x or self.transform.x
	self.transform.y = y or self.transform.y
	self.transform.z = z or self.transform.z
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

function Object:set_rotation(r)
	self.transform.rotation = r
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

function Object:set_scale(s)
	self.transform.scale = s
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

function Object:set_layer(layer)
	self.layer = layer
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

function Object:set_safe_area(enabled)
	self.safe_area = enabled
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

-- Hierarchy (Chapter 4)
function Object:add_child(child)
	if type(child) == "string" then
		local session = cci.get_session(self.player_name)
		child = session.objects[child]
	end
	if child then
		child.parent = self.id
		table.insert(self.children, child.id)
		local session = cci.get_session(self.player_name)
		session.is_dirty = true
	end
	return self
end

-- Detach a child
function Object:remove_child(child)
	if type(child) == "string" then
		local session = cci.get_session(self.player_name)
		child = session.objects[child]
	end
	if child then
		for i, cid in ipairs(self.children) do
			if cid == child.id then
				table.remove(self.children, i)
				child.parent = nil
				local session = cci.get_session(self.player_name)
				session.is_dirty = true
				break
			end
		end
	end
	return self
end

-- Functionable Services: Events, Conditions & Actions (Chapter 1)
function Object:on(event_name, condition_fn, action_fn)
	-- If only event_name and action_fn are provided, condition_fn is optional
	if not action_fn then
		action_fn = condition_fn
		condition_fn = nil
	end

	self.events[event_name] = self.events[event_name] or {}
	table.insert(self.events[event_name], {
		condition = condition_fn,
		action = action_fn
	})
	return self
end

function Object:trigger(event_name, data)
	local handlers = self.events[event_name]
	if handlers then
		for _, handler in ipairs(handlers) do
			local proceed = true
			if handler.condition then
				proceed = handler.condition(self, data)
			end
			if proceed then
				handler.action(self, data)
			end
		end
	end
end

-- Visibility & State
function Object:show()
	self.visible = true
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

function Object:hide()
	self.visible = false
	local session = cci.get_session(self.player_name)
	session.is_dirty = true
	return self
end

-- Destruction
function Object:destroy()
	local session = cci.get_session(self.player_name)

	-- Destroy all children first
	local children_to_destroy = {unpack(self.children)}
	for _, cid in ipairs(children_to_destroy) do
		local child = session.objects[cid]
		if child then
			child:destroy()
		end
	end

	-- Remove from parent
	if self.parent then
		local parent_obj = session.objects[self.parent]
		if parent_obj then
			parent_obj:remove_child(self)
		end
	end

	-- Trigger destruction event
	self:trigger("destroy")

	session.objects[self.id] = nil
	session.is_dirty = true
end
