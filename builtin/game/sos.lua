
local sos_callbacks = {} -- [guid] = { [event] = {funcs} }
local sos_objects = {} -- [guid] = ObjectRef

local function get_callbacks(obj)
	local guid = obj:get_guid()
	if not sos_callbacks[guid] then
		sos_callbacks[guid] = {}
	end
	return sos_callbacks[guid]
end

local old_add_scene_object = core.add_scene_object
core.add_scene_object = function(params)
	local obj = old_add_scene_object(params)
	if not obj then return nil end

	local guid = obj:get_guid()
	sos_callbacks[guid] = {}
	sos_objects[guid] = obj

	function obj:on(event, func)
		local cb = get_callbacks(self)
		if not cb[event] then cb[event] = {} end
		table.insert(cb[event], func)
	end

	function obj:fire(event, data)
		local cb = get_callbacks(self)
		if cb[event] then
			for _, func in ipairs(cb[event]) do
				func(self, data)
			end
		end
	end

	function obj:destroy()
		self:fire("destroy")
		local guid = self:get_guid()
		sos_callbacks[guid] = nil
		sos_objects[guid] = nil
		self:remove()
	end

	-- Hierarchy helpers
	function obj:attach(parent)
		self:set_attach(parent, "", {x=0,y=0,z=0}, {x=0,y=0,z=0})
	end

	function obj:detach()
		self:set_detach()
	end

	function obj:get_parent()
		local p = self:get_attach()
		return p -- get_attach returns parent, bone, pos, rot
	end

	return obj
end
