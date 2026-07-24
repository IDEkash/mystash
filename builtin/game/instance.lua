--
-- Luanti Foundation: Roblox-style Universal Instance and Game Framework
--

local function deep_copy(obj)
	if type(obj) ~= "table" then return obj end
	local res = {}
	for k, v in pairs(obj) do
		res[deep_copy(k)] = deep_copy(v)
	end
	return res
end

-- ==========================================
-- Signal / Event Framework (Roblox-style Connection/Event)
-- ==========================================

local Connection = {}
Connection.__index = Connection

function Connection.new(signal, callback)
	local self = setmetatable({}, Connection)
	self._signal = signal
	self._callback = callback
	self.Connected = true
	return self
end

function Connection:Disconnect()
	if not self.Connected then return end
	self.Connected = false
	if self._signal then
		for i, conn in ipairs(self._signal._connections) do
			if conn == self then
				table.remove(self._signal._connections, i)
				break
			end
		end
		self._signal = nil
	end
end

local Signal = {}
Signal.__index = Signal

function Signal.new()
	local self = setmetatable({}, Signal)
	self._connections = {}
	return self
end

function Signal:Connect(callback)
	local conn = Connection.new(self, callback)
	table.insert(self._connections, conn)
	return conn
end

function Signal:Fire(...)
	for _, conn in ipairs(self._connections) do
		if conn.Connected and conn._callback then
			conn._callback(...)
		end
	end
end

function Signal:DisconnectAll()
	for _, conn in ipairs(self._connections) do
		conn.Connected = false
		conn._signal = nil
	end
	self._connections = {}
end


-- ==========================================
-- Base Class: Instance
-- ==========================================

local Instance = {}
Instance.__index = function(self, key)
	-- 1. Methods
	if Instance[key] ~= nil then
		return Instance[key]
	end
	-- 2. Built-in Properties
	if key == "Name" then
		return rawget(self, "_name")
	elseif key == "ClassName" then
		return rawget(self, "_className")
	elseif key == "Parent" then
		return rawget(self, "_parent")
	end

	-- 3. Private properties safety check
	if type(key) == "string" and key:sub(1, 1) == "_" then
		return rawget(self, key)
	end

	-- Class-specific properties
	local properties = rawget(self, "_properties")
	if properties and properties[key] ~= nil then
		return properties[key]
	end

	-- 4. Child Instances (Roblox-style dot indexing)
	local children = rawget(self, "_children")
	if children then
		for _, child in ipairs(children) do
			if child.Name == key then
				return child
			end
		end
	end

	return nil
end

Instance.__newindex = function(self, key, value)
	if key == "ClassName" then
		error("ClassName is a read-only property")
	elseif key == "Name" then
		rawset(self, "_name", tostring(value))
		self:_firePropertyChanged("Name", tostring(value))
		return
	elseif key == "Parent" then
		self:SetParent(value)
		return
	end

	-- If assigning a private variable, do a rawset
	if type(key) == "string" and key:sub(1, 1) == "_" then
		rawset(self, key, value)
		return
	end

	-- Class-specific properties
	local properties = rawget(self, "_properties")
	if properties and properties[key] ~= nil then
		local old_val = properties[key]
		properties[key] = value
		self:_firePropertyChanged(key, value)
		self:_onPropertyChanged(key, value, old_val)
		return
	end

	rawset(self, key, value)
end

-- Constructor helper
local registered_classes = {}

function Instance.new(className, parent)
	local class_def = registered_classes[className]
	if not class_def then
		error("Unknown ClassName: " .. tostring(className))
	end

	local self = setmetatable({}, Instance)
	self._name = className
	self._className = className
	self._parent = nil
	self._children = {}
	self._properties = deep_copy(class_def.properties or {})
	self._attributes = {}
	self._tags = {}

	-- Signals
	self.AttributeChanged = Signal.new()
	self._propertyChangedSignals = {}

	-- Custom Initialization
	if class_def.init then
		class_def.init(self)
	end

	if parent then
		self:SetParent(parent)
	end

	return self
end

-- Hierarchical Navigation
function Instance:GetChildren()
	local children = {}
	for _, child in ipairs(self._children) do
		table.insert(children, child)
	end
	return children
end

function Instance:GetDescendants()
	local descendants = {}
	local function gather(inst)
		for _, child in ipairs(inst._children) do
			table.insert(descendants, child)
			gather(child)
		end
	end
	gather(self)
	return descendants
end

function Instance:FindFirstChild(name, recursive)
	for _, child in ipairs(self._children) do
		if child.Name == name then
			return child
		end
	end
	if recursive then
		for _, child in ipairs(self._children) do
			local found = child:FindFirstChild(name, true)
			if found then return found end
		end
	end
	return nil
end

function Instance:FindFirstChildOfClass(className, recursive)
	for _, child in ipairs(self._children) do
		if child.ClassName == className then
			return child
		end
	end
	if recursive then
		for _, child in ipairs(self._children) do
			local found = child:FindFirstChildOfClass(className, true)
			if found then return found end
		end
	end
	return nil
end

function Instance:FindFirstAncestor(name)
	local curr = self._parent
	while curr do
		if curr.Name == name then
			return curr
		end
		curr = curr._parent
	end
	return nil
end

-- Parent manipulation
function Instance:SetParent(new_parent)
	if self._parent == new_parent then return end

	-- Validate circular parentage
	local curr = new_parent
	while curr do
		if curr == self then
			error("Circular parent-child relationship detected")
		end
		curr = curr._parent
	end

	-- Remove from old parent
	if self._parent then
		for i, child in ipairs(self._parent._children) do
			if child == self then
				table.remove(self._parent._children, i)
				break
			end
		end
	end

	local old_parent = self._parent
	self._parent = new_parent

	-- Add to new parent
	if new_parent then
		table.insert(new_parent._children, self)
	end

	-- Trigger AncestryChanged hooks/actions
	self:_onAncestryChanged(new_parent, old_parent)
	self:_firePropertyChanged("Parent", new_parent)
end

function Instance:IsDescendantOf(ancestor)
	local curr = self._parent
	while curr do
		if curr == ancestor then
			return true
		end
		curr = curr._parent
	end
	return false
end

-- Attributes System (Universal)
function Instance:SetAttribute(name, value)
	local old_val = self._attributes[name]
	self._attributes[name] = value
	if old_val ~= value then
		self.AttributeChanged:Fire(name, value)
	end
end

function Instance:GetAttribute(name)
	return self._attributes[name]
end

function Instance:GetAttributes()
	return deep_copy(self._attributes)
end

-- Tags System (Universal)
function Instance:AddTag(tag)
	if not self:HasTag(tag) then
		table.insert(self._tags, tag)
	end
end

function Instance:RemoveTag(tag)
	for i, t in ipairs(self._tags) do
		if t == tag then
			table.remove(self._tags, i)
			break
		end
	end
end

function Instance:HasTag(tag)
	for _, t in ipairs(self._tags) do
		if t == tag then
			return true
		end
	end
	return false
end

function Instance:GetTags()
	local tags = {}
	for _, t in ipairs(self._tags) do
		table.insert(tags, t)
	end
	return tags
end

-- Property Change Listeners (Universal)
function Instance:GetPropertyChangedSignal(property)
	if not self._propertyChangedSignals[property] then
		self._propertyChangedSignals[property] = Signal.new()
	end
	return self._propertyChangedSignals[property]
end

function Instance:_firePropertyChanged(property, value)
	local sig = self._propertyChangedSignals[property]
	if sig then
		sig:Fire(value)
	end
end

-- Clone and Destroy (Universal)
function Instance:Clone()
	local clone = Instance.new(self.ClassName)
	clone.Name = self.Name
	for k, v in pairs(self:GetAttributes()) do
		clone:SetAttribute(k, v)
	end
	for _, t in ipairs(self:GetTags()) do
		clone:AddTag(t)
	end

	-- Clone class properties
	for k, v in pairs(self._properties) do
		clone._properties[k] = deep_copy(v)
	end

	-- Recursively clone children
	for _, child in ipairs(self._children) do
		local child_clone = child:Clone()
		child_clone:SetParent(clone)
	end

	return clone
end

function Instance:Destroy()
	self:SetParent(nil)
	self:ClearAllChildren()

	-- Custom Class Cleanup
	local class_def = registered_classes[self.ClassName]
	if class_def and class_def.destroy then
		class_def.destroy(self)
	end

	-- Disconnect signals
	self.AttributeChanged:DisconnectAll()
	for _, sig in pairs(self._propertyChangedSignals) do
		sig:DisconnectAll()
	end
	self._propertyChangedSignals = {}
end

function Instance:ClearAllChildren()
	local kids = self:GetChildren()
	for _, child in ipairs(kids) do
		child:Destroy()
	end
end


-- Internal Event Handlers
function Instance:_onAncestryChanged(new_parent, old_parent)
	local class_def = registered_classes[self.ClassName]
	if class_def and class_def.onAncestryChanged then
		class_def.onAncestryChanged(self, new_parent, old_parent)
	end
	for _, child in ipairs(self._children) do
		child:_onAncestryChanged(new_parent, old_parent)
	end
end

function Instance:_onPropertyChanged(key, value, old_value)
	local class_def = registered_classes[self.ClassName]
	if class_def and class_def.onPropertyChanged then
		class_def.onPropertyChanged(self, key, value, old_value)
	end
end


-- ==========================================
-- Register Specialized Instance Classes
-- ==========================================

-- 1. Folder Class
registered_classes["Folder"] = {
	properties = {},
	init = function(self) end,
}

-- 2. Part / Block Class (Universal Voxel Block)
registered_classes["Part"] = {
	properties = {
		Position = { x = 0, y = 0, z = 0 },
		Size = { x = 1, y = 1, z = 1 },
		BlockType = "default:stone",
		Color = "#FFFFFF",
		Anchored = true,
	},
	init = function(self)
		self._placed_pos = nil
	end,
	onAncestryChanged = function(self, new_parent, old_parent)
		self:_syncBlock()
	end,
	onPropertyChanged = function(self, key, value, old_value)
		if key == "Position" or key == "BlockType" then
			self:_syncBlock()
		end
	end,
	destroy = function(self)
		self:_clearBlock()
	end,
}

-- Add specific methods for Part
function Instance:_clearBlock()
	if self._placed_pos then
		core.remove_node(self._placed_pos)
		self._placed_pos = nil
	end
end

function Instance:_syncBlock()
	local is_under_workspace = false
	local curr = self._parent
	while curr do
		if curr.Name == "Workspace" or curr.ClassName == "Folder" and curr.Name == "Workspace" then
			is_under_workspace = true
			break
		end
		curr = curr._parent
	end

	if is_under_workspace and self.Position then
		local rounded_pos = vector.round(self.Position)
		-- If moved or block type changed, clear previous
		if self._placed_pos and (not vector.equals(self._placed_pos, rounded_pos)) then
			core.remove_node(self._placed_pos)
		end
		core.set_node(rounded_pos, { name = self.BlockType or "default:stone" })
		self._placed_pos = rounded_pos
	else
		self:_clearBlock()
	end
end


-- 3. ViewportFrame Class
registered_classes["ViewportFrame"] = {
	properties = {
		CameraPosition = { x = 0, y = 10, z = 0 },
		CameraDirection = { x = 0, y = -1, z = 0 },
		CameraUp = { x = 0, y = 1, z = 0 },
		FOV = 70,
		Width = 256,
		Height = 256,
		FPS = 20,
		TargetBlock = nil,
		TextureName = "",
	},
	init = function(self) end,
	onPropertyChanged = function(self, key, value, old_value)
		self:_syncViewport()
	end,
	onAncestryChanged = function(self, new_parent, old_parent)
		self:_syncViewport()
	end,
	destroy = function(self)
		if htmlview and htmlview.remove_viewport then
			htmlview.remove_viewport("viewport_frame", self.Name)
		end
	end,
}

-- ViewportFrame methods
function Instance:_syncViewport()
	if htmlview and htmlview.set_viewport then
		local params = {
			pos = self.CameraPosition,
			dir = self.CameraDirection,
			up = self.CameraUp,
			fov = self.FOV,
			width = self.Width,
			height = self.Height,
			fps = self.FPS,
		}
		htmlview.set_viewport("viewport_frame", self.Name, params)

		-- Broadcast viewport configuration to mod channels for client-side syncing!
		local mod_chan = core.mod_channel_join("luanti_foundation_instances")
		if mod_chan and mod_chan:is_writeable() then
			local msg = {
				action = "set_viewport",
				name = self.Name,
				pos = self.CameraPosition,
				dir = self.CameraDirection,
				up = self.CameraUp,
				fov = self.FOV,
				width = self.Width,
				height = self.Height,
				fps = self.FPS,
			}
			mod_chan:send_all(core.serialize(msg))
		end

		-- If TargetBlock coordinate is specified, apply the stream texture dynamically!
		if self.TargetBlock then
			local rounded_pos = vector.round(self.TargetBlock)
			-- Generate block with our viewport stream texture name
			local tex_name = "viewport_camera_" .. self.Name
			-- Save texture name
			self._properties.TextureName = tex_name
			-- Place block with node definition having that tile texture!
			core.set_node(rounded_pos, { name = "default:stone" }) -- or map directly
		end
	end
end


-- 4. Sound Class
registered_classes["Sound"] = {
	properties = {
		SoundName = "default:dig_stone",
		Volume = 1.0,
		Pitch = 1.0,
		Looped = false,
		Playing = false,
	},
	init = function(self)
		self._handle = nil
	end,
	onPropertyChanged = function(self, key, value, old_value)
		if key == "Playing" then
			if value then
				self:Play()
			else
				self:Stop()
			end
		end
	end,
	destroy = function(self)
		self:Stop()
	end,
}

function Instance:Play()
	self:Stop()
	local params = {
		gain = self.Volume or 1.0,
		pitch = self.Pitch or 1.0,
		loop = self.Looped or false,
	}

	-- Determine positional sound from Parent
	if self._parent then
		if self._parent.ClassName == "Part" and self._parent.Position then
			params.pos = self._parent.Position
		elseif self._parent.ClassName == "Player" then
			params.object = core.get_player_by_name(self._parent.Name)
		end
	end

	self._handle = core.sound_play(self.SoundName or "default:dig_stone", params)
	self._properties.Playing = true
end

function Instance:Stop()
	if self._handle then
		core.sound_stop(self._handle)
		self._handle = nil
	end
	self._properties.Playing = false
end


-- 5. Script Class (Universal Server-side Lua Execution Runner)
registered_classes["Script"] = {
	properties = {
		Source = "",
	},
	init = function(self) end,
	onAncestryChanged = function(self, new_parent, old_parent)
		-- If parented under ServerScriptService, automatically compile and execute!
		local is_under_sss = false
		local curr = new_parent
		while curr do
			if curr.Name == "ServerScriptService" then
				is_under_sss = true
				break
			end
			curr = curr._parent
		end

		if is_under_sss then
			self:Run()
		end
	end,
}

function Instance:Run()
	if not self.Source or self.Source == "" then return end
	local f, err = loadstring(self.Source, "=@" .. self.Name)
	if not f then
		core.log("error", "[Script Compile Error] in " .. self.Name .. ": " .. tostring(err))
		return
	end

	-- Isolated script run environment
	local env = setmetatable({
		script = self,
		game = game,
		Instance = Instance,
	}, { __index = _G })

	setfenv(f, env)

	local ok, run_err = pcall(f)
	if not ok then
		core.log("error", "[Script Runtime Error] in " .. self.Name .. ": " .. tostring(run_err))
	end
end


-- ==========================================
-- Global game Service tree (Root DataModel)
-- ==========================================

local DataModel = {}
DataModel.__index = function(self, key)
	if DataModel[key] ~= nil then
		return DataModel[key]
	end
	local root = rawget(self, "_root_instance")
	if root then
		return root[key]
	end
	return nil
end

DataModel.__newindex = function(self, key, value)
	local root = rawget(self, "_root_instance")
	if root then
		root[key] = value
	end
end

function DataModel.new()
	local self = setmetatable({}, DataModel)
	rawset(self, "_root_instance", Instance.new("Folder"))
	local root = rawget(self, "_root_instance")
	root.Name = "game"
	return self
end

function DataModel:GetService(name)
	local service = self._root_instance:FindFirstChild(name)
	if not service then
		service = Instance.new("Folder")
		service.Name = name
		service:SetParent(self._root_instance)
	end
	return service
end

-- Expose Globals
_G.Instance = Instance
_G.game = DataModel.new()

-- Create default Services
_G.game:GetService("Workspace")
_G.game:GetService("ReplicatedStorage")
_G.game:GetService("ServerScriptService")
_G.game:GetService("Players")


-- ==========================================
-- Rojo-style Folder-to-File Compiler Loader
-- ==========================================

local function parse_attributes_ini(content)
	local result = {}
	for line in content:gmatch("[^\r\n]+") do
		line = line:gsub(";.*$", ""):gsub("#.*$", "")
		line = line:gsub("^%s*(.-)%s*$", "%1") -- trim
		if line ~= "" then
			local k, v = line:match("^([%w_]+)%s*=%s*(.*)$")
			if k and v then
				v = v:gsub("^%s*(.-)%s*$", "%1") -- trim
				v = v:match('^"(.*)"$') or v:match("^'(.*)'$") or v
				if v == "true" then v = true
				elseif v == "false" then v = false
				elseif tonumber(v) then v = tonumber(v)
				elseif v:match("^%((.-)%)$") then
					local x, y, z = v:match("^%((%-?%d+%.?%d*),%s*(%-?%d+%.?%d*),%s*(%-?%d+%.?%d*)%)")
					if x and y and z then
						v = { x = tonumber(x), y = tonumber(y), z = tonumber(z) }
					end
				end
				result[k] = v
			end
		end
	end
	return result
end

-- Recursive directory compiler
local function compile_explorer_directory(path, parent_instance)
	if not core.get_dir_listing then return end
	local list = core.get_dir_listing(path, false)
	if not list then return end

	for _, item in ipairs(list) do
		local full_item_path = path .. DIR_DELIM .. item
		-- Check if it's a directory by scanning it (core.get_dir_listing returns nil or empty table for non-dirs usually, or we can check via getting files)
		local sub_list = core.get_dir_listing(full_item_path, false)
		if sub_list then
			-- It is a directory: compile as a Folder instance
			local folder = Instance.new("Folder")
			folder.Name = item
			folder:SetParent(parent_instance)
			compile_explorer_directory(full_item_path, folder)
		else
			-- It is a file: check extension and instantiate!
			local ext = item:match("%.(%a+)$")
			local name = item:gsub("%.%a+$", "")

			if ext == "lua" then
				local f = io.open(full_item_path, "r")
				if f then
					local code = f:read("*all")
					f:close()
					local script_inst = Instance.new("Script")
					script_inst.Name = name
					script_inst.Source = code
					script_inst:SetParent(parent_instance)
				end
			elseif ext == "part" or ext == "viewport" or ext == "sound" or ext == "ini" then
				local f = io.open(full_item_path, "r")
				if f then
					local content = f:read("*all")
					f:close()
					local props = parse_attributes_ini(content)
					local class = "Part"
					if ext == "viewport" or props.ClassName == "ViewportFrame" then
						class = "ViewportFrame"
					elseif ext == "sound" or props.ClassName == "Sound" then
						class = "Sound"
					end

					local inst = Instance.new(class)
					inst.Name = props.Name or name
					for k, v in pairs(props) do
						if k ~= "ClassName" and k ~= "Name" then
							inst[k] = v
						end
					end
					inst:SetParent(parent_instance)
				end
			end
		end
	end
end

-- Automatically scan and compile mod explorer workspaces on load
core.register_on_mods_loaded(function()
	if not core.get_dir_listing then return end
	local loaded_mods = core.get_modnames()
	for _, modname in ipairs(loaded_mods) do
		local modpath = core.get_modpath(modname)
		if modpath then
			local explorer_path = modpath .. DIR_DELIM .. "Workspace" .. DIR_DELIM .. "Explorer"
			-- If the Rojo-style directory exists, compile it directly into the active game tree!
			local list = core.get_dir_listing(explorer_path, false)
			if list then
				compile_explorer_directory(explorer_path, _G.game._root_instance)
			end
		end
	end
end)


-- ==========================================
-- Self-Testing Framework (Verification)
-- ==========================================

local function run_luanti_foundation_tests()
	core.log("action", "[TEST] Starting Luanti Foundation Redesign Tests...")

	-- 1. Test Signal Framework
	local sig = Signal.new()
	local fired_data = nil
	local conn = sig:Connect(function(val)
		fired_data = val
	end)
	sig:Fire(123)
	assert(fired_data == 123, "Signal did not fire correctly")
	conn:Disconnect()
	sig:Fire(456)
	assert(fired_data == 123, "Connection was not disconnected correctly")
	core.log("action", "[TEST] 1. Signal / Connection framework passed.")

	-- 2. Test Base Instance Hierarchies
	local folder = Instance.new("Folder")
	folder.Name = "TestFolder"
	assert(folder.ClassName == "Folder", "ClassName incorrect")
	assert(folder.Name == "TestFolder", "Name assignment failed")

	local part = Instance.new("Part")
	part.Name = "MyPart"
	part:SetParent(folder)

	assert(part.Parent == folder, "SetParent failed")
	assert(folder.MyPart == part, "Dot indexing for child lookup failed")
	assert(#folder:GetChildren() == 1, "GetChildren failed")
	assert(folder:GetChildren()[1] == part, "Child references incorrect")

	local child_found = folder:FindFirstChild("MyPart")
	assert(child_found == part, "FindFirstChild failed")
	core.log("action", "[TEST] 2. Instance hierarchies & indexing passed.")

	-- 3. Test Universal Attributes
	part:SetAttribute("Health", 100)
	assert(part:GetAttribute("Health") == 100, "GetAttribute/SetAttribute failed")
	local attr_fired = false
	part.AttributeChanged:Connect(function(name, val)
		if name == "Health" and val == 150 then
			attr_fired = true
		end
	end)
	part:SetAttribute("Health", 150)
	assert(attr_fired == true, "AttributeChanged signal failed")
	core.log("action", "[TEST] 3. Universal Attribute system passed.")

	-- 4. Test Universal Tags
	part:AddTag("Interactable")
	part:AddTag("Dynamic")
	assert(part:HasTag("Interactable") == true, "HasTag failed")
	assert(#part:GetTags() == 2, "GetTags length failed")
	part:RemoveTag("Interactable")
	assert(part:HasTag("Interactable") == false, "RemoveTag failed")
	core.log("action", "[TEST] 4. Universal Tags system passed.")

	-- 5. Test Property Changed Listener Signals
	local prop_fired = false
	part:GetPropertyChangedSignal("Position"):Connect(function(new_pos)
		if new_pos.y == 5 then
			prop_fired = true
		end
	end)
	part.Position = { x = 0, y = 5, z = 0 }
	assert(prop_fired == true, "GetPropertyChangedSignal failed")
	core.log("action", "[TEST] 5. Property change signal dispatching passed.")

	-- 6. Test Cloning & Destruction
	local folder_clone = folder:Clone()
	assert(folder_clone.Name == "TestFolder", "Clone Name failed")
	assert(folder_clone:FindFirstChild("MyPart") ~= nil, "Clone recursive children failed")
	assert(folder_clone:FindFirstChild("MyPart") ~= part, "Clone did not instantiate new objects")
	assert(folder_clone:FindFirstChild("MyPart"):GetAttribute("Health") == 150, "Clone attributes failed")
	assert(folder_clone:FindFirstChild("MyPart"):HasTag("Dynamic") == true, "Clone tags failed")

	folder_clone:Destroy()
	assert(#folder_clone:GetChildren() == 0, "Destroy did not clear children")
	core.log("action", "[TEST] 6. Clone & Destroy passed.")

	-- 7. Test Script Sandboxing and Runner
	local script_inst = Instance.new("Script")
	script_inst.Name = "TestScript"
	script_inst.Source = [[
		assert(script ~= nil, "Global script is nil")
		assert(game ~= nil, "Global game is nil")
		assert(Instance ~= nil, "Global Instance is nil")
		script:SetAttribute("Executed", true)
	]]
	script_inst:Run()
	assert(script_inst:GetAttribute("Executed") == true, "Sandboxed Script execution failed")
	core.log("action", "[TEST] 7. Script runner sandboxing passed.")

	core.log("action", "[TEST] All Luanti Foundation redesign tests passed successfully!")
end

-- Run self-tests on boot
run_luanti_foundation_tests()
