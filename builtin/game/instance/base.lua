-- Roblox-Style Instance Core Base Class (Built-in)
-- Manages the Class registrations, properties, metatables, custom getters/setters, and tree hierarchy.

-- Signal class (Roblox-style RBXScriptSignal)
Signal = {}
Signal.__index = Signal

function Signal.new()
	return setmetatable({ _listeners = {} }, Signal)
end

function Signal:Connect(callback)
	table.insert(self._listeners, callback)
	return {
		Disconnect = function()
			for i, cb in ipairs(self._listeners) do
				if cb == callback then
					table.remove(self._listeners, i)
					break
				end
			end
		end
	}
end

function Signal:Fire(...)
	for _, cb in ipairs(self._listeners) do
		pcall(cb, ...)
	end
end


Instance = {
	classes = {},
}

local next_ref = 1

function Instance.new(className, parent)
	local class_impl = Instance.classes[className] or {}

	-- Internal private storage to support custom getters/setters without infinite recursion
	local _properties = {
		ClassName = className,
		Name = className,
		Parent = nil,
		_children = {},
		_ref = "ref_" .. next_ref,
		_destroyed = false,
	}
	next_ref = next_ref + 1

	-- Merge with default class properties
	if class_impl.default_properties then
		for k, v in pairs(class_impl.default_properties) do
			_properties[k] = v
		end
	end

	local obj = {}

	-- Signals
	_properties.Changed = Signal.new() -- Public changed signal (Connect to it!)
	_properties._property_changed_signal = Signal.new() -- Private sync signal

	-- Custom Metatable to support Roblox-style dot syntax for properties
	local mt = {
		_properties = _properties, -- Store private reference directly inside the metatable for Destroy() access
		__index = function(t, key)
			-- Check custom methods first
			if class_impl[key] then
				return class_impl[key]
			elseif Instance[key] then
				return Instance[key]
			end
			-- Return from private properties
			return _properties[key]
		end,

		__newindex = function(t, key, value)
			if _properties._destroyed then
				error("Attempt to modify a destroyed Instance")
			end

			-- Read-only system properties
			if key == "ClassName" or key == "_ref" or key == "_children" or key == "Changed" or key == "_property_changed_signal" then
				error("Property " .. key .. " is read-only")
			end

			local old_value = _properties[key]
			if old_value == value then return end

			-- Intercept Parent changes to rebuild the hierarchy (Roblox-style)
			if key == "Parent" then
				local old_parent = _properties.Parent
				if old_parent == value then return end

				-- 1. Remove from old parent
				if old_parent then
					local old_children = old_parent._children
					for i, child in ipairs(old_children) do
						if child == t then
							table.remove(old_children, i)
							break
						end
					end
				end

				-- 2. Set new parent
				_properties.Parent = value

				-- 3. Add to new parent
				if value then
					table.insert(value._children, t)
				end

				-- Trigger UI update if it is a GUI element
				if game.ui_runtime then
					game.ui_runtime.mark_instance_dirty(t)
				end

			else
				-- Standard property update
				_properties[key] = value

				-- Trigger UI update if it is a GUI element
				if game.ui_runtime then
					game.ui_runtime.mark_instance_dirty(t)
				end
			end

			-- Fire private sync signal (key, value, old_value)
			_properties._property_changed_signal:Fire(key, value, old_value)

			-- Fire public Changed signal (propertyName)
			_properties.Changed:Fire(key)
		end,

		__tostring = function(t)
			return _properties.Name .. " (" .. _properties.ClassName .. ")"
		end
	end

	setmetatable(obj, mt)

	-- Run custom initialization of specific classes
	if class_impl.init then
		class_impl.init(obj)
	end

	-- Set initial parent
	if parent then
		obj.Parent = parent
	end

	return obj
end

-- Base methods shared by all Instances (Roblox-style)
function Instance:GetChildren()
	local copy = {}
	for i, child in ipairs(self._children) do
		copy[i] = child
	end
	return copy
end

function Instance:FindFirstChild(name)
	for _, child in ipairs(self._children) do
		if child.Name == name then
			return child
		end
	end
	return nil
end

function Instance:FindFirstChildOfClass(className)
	for _, child in ipairs(self._children) do
		if child.ClassName == className then
			return child
		end
	end
	return nil
end

function Instance:Destroy()
	local mt = getmetatable(self)
	if not mt then return end

	-- Mark as destroyed using direct metatable properties lookup
	local props = mt._properties
	if not props or props._destroyed then return end
	props._destroyed = true

	-- 1. Detach from parent
	self.Parent = nil

	-- 2. Recursively destroy all children first
	local children = self:GetChildren()
	for _, child in ipairs(children) do
		child:Destroy()
	end

	-- 3. Lock metatable indexes to raise descriptive errors on further reads/writes
	mt.__index = function(t, k)
		error("Attempt to index a destroyed Instance (" .. k .. ")")
	end
	mt.__newindex = function(t, k, v)
		error("Attempt to modify a destroyed Instance (" .. k .. ")")
	end
end
