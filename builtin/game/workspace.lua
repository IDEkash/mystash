--
-- Unified Engine Architecture Workspace Loader
--
-- Implements Part III of the spec: Workspace Definitions
-- Automatically scans and registers .block, .character, .item, .vehicle,
-- .pathfinding, .structure, .rig, and .ui definitions from Workspace/
--

local function trim(s)
	return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local function parse_ini(content)
	local result = {}
	local stack = { result }
	local current = result

	for line in content:gmatch("[^\r\n]+") do
		line = line:gsub(";.*$", "") -- strip comments starting with ;
		line = line:gsub("#.*$", "") -- strip comments starting with #
		line = trim(line)
		if line ~= "" then
			if line:match("^{") then
				-- opening brace
			elseif line:match("^}") then
				-- closing brace
				if #stack > 1 then
					table.remove(stack)
					current = stack[#stack]
				end
			else
				local key = line:match("^([%w_]+)%s*{")
				if key then
					local tbl = {}
					current[key] = tbl
					table.insert(stack, tbl)
					current = tbl
				else
					local k, v = line:match("^([%w_]+)%s*=%s*(.*)$")
					if k and v then
						v = trim(v)
						v = v:match('^"(.*)"$') or v:match("^'(.*)'$") or v
						if v == "true" then
							v = true
						elseif v == "false" then
							v = false
						elseif tonumber(v) then
							v = tonumber(v)
						elseif v:match("^%((.-)%)$") then
							local x, y, z = v:match("^%((%-?%d+%.?%d*),%s*(%-?%d+%.?%d*),%s*(%-?%d+%.?%d*)%)")
							if x and y and z then
								v = { x = tonumber(x), y = tonumber(y), z = tonumber(z) }
							end
						end
						current[k] = v
					else
						local item = line:match('^"(.*)"$') or line:match("^'(.*)'$") or line
						table.insert(current, item)
					end
				end
			end
		end
	end
	return result
end

local function parse_structure(content)
	local data = {}
	local palette = {}
	local layers = {}
	local spawn_points = {}
	local markers = {}
	local settings = {}

	local current_layer = nil
	local current_layer_content = {}

	for line in content:gmatch("[^\r\n]+") do
		line = trim(line)
		if line ~= "" then
			local l_name = line:match('^Layer%s+"(.-)"') or line:match("^Layer%s+(.-)$")
			if l_name then
				if current_layer then
					layers[current_layer] = table.concat(current_layer_content, "\n")
				end
				current_layer = l_name
				current_layer_content = {}
			elseif current_layer then
				table.insert(current_layer_content, line)
			else
				local k, v = line:match("^([%w_]+)%s*=%s*(.*)$")
				if k and v then
					v = trim(v)
					v = v:match('^"(.*)"$') or v:match("^'(.*)'$") or v
					if v == "true" then v = true
					elseif v == "false" then v = false
					elseif tonumber(v) then v = tonumber(v)
					end
					data[k] = v
				end
			end
		end
	end

	if current_layer then
		layers[current_layer] = table.concat(current_layer_content, "\n")
	end

	-- Extract Palette
	local pal_pos = content:find("Palette")
	if pal_pos then
		local start_brace = content:find('{', pal_pos)
		local end_brace = content:find('}', pal_pos)
		if start_brace and end_brace and end_brace > start_brace then
			local block = content:sub(start_brace + 1, end_brace - 1)
			for line in block:gmatch("[^\r\n]+") do
				local k, v = line:match("^%s*([%w_]+)%s*=%s*([%w_]+)")
				if k and v then
					palette[tonumber(k) or k] = v
				end
			end
		end
	end

	-- Extract SpawnPoints
	local sp_pos = content:find("SpawnPoints")
	if sp_pos then
		local start_brace = content:find('{', sp_pos)
		local end_brace = content:find('}', sp_pos)
		if start_brace and end_brace and end_brace > start_brace then
			local block = content:sub(start_brace + 1, end_brace - 1)
			for line in block:gmatch("[^\r\n]+") do
				local k, v = line:match("^%s*([%w_]+)%s*=%s*(.*)$")
				if k and v then
					v = trim(v)
					local x, y, z = v:match("^%((%-?%d+%.?%d*),%s*(%-?%d+%.?%d*),%s*(%-?%d+%.?%d*)%)")
					if x and y and z then
						spawn_points[k] = { x = tonumber(x), y = tonumber(y), z = tonumber(z) }
					end
				end
			end
		end
	end

	-- Extract Markers
	local mk_pos = content:find("Markers")
	if mk_pos then
		local start_brace = content:find('{', mk_pos)
		local end_brace = content:find('}', mk_pos)
		if start_brace and end_brace and end_brace > start_brace then
			local block = content:sub(start_brace + 1, end_brace - 1)
			for line in block:gmatch("[^\r\n]+") do
				local k, v = line:match("^%s*([%w_]+)%s*=%s*(.*)$")
				if k and v then
					v = trim(v)
					local x, y, z = v:match("^%((%-?%d+%.?%d*),%s*(%-?%d+%.?%d*),%s*(%-?%d+%.?%d*)%)")
					if x and y and z then
						markers[k] = { x = tonumber(x), y = tonumber(y), z = tonumber(z) }
					end
				end
			end
		end
	end

	-- Extract Settings
	local st_pos = content:find("Settings")
	if st_pos then
		local start_brace = content:find('{', st_pos)
		local end_brace = content:find('}', st_pos)
		if start_brace and end_brace and end_brace > start_brace then
			local block = content:sub(start_brace + 1, end_brace - 1)
			for line in block:gmatch("[^\r\n]+") do
				local k, v = line:match("^%s*([%w_]+)%s*=%s*(.*)$")
				if k and v then
					v = trim(v)
					if v == "true" then v = true
					elseif v == "false" then v = false
					elseif tonumber(v) then v = tonumber(v)
					end
					settings[k] = v
				end
			end
		end
	end

	data.layers = layers
	data.Palette = palette
	data.SpawnPoints = spawn_points
	data.Markers = markers
	data.Settings = settings
	return data
end

-- Registries
local registered_structures = {}
local registered_pathfindings = {}
local registered_rigs = {}
local registered_effects = {}
local registered_lights = {}
local registered_uis = {}

-- Directory scanning helper
local function scan_workspace_dir(modpath, subfolder, ext)
	local files = {}
	local full_path = modpath .. DIR_DELIM .. "Workspace" .. DIR_DELIM .. subfolder
	local list = core.get_dir_listing and core.get_dir_listing(full_path, false)
	if not list then
		list = core.get_dir_listing and core.get_dir_listing(full_path)
	end
	if list then
		for _, name in ipairs(list) do
			if name:sub(-#ext) == ext then
				table.insert(files, { name = name, fullpath = full_path .. DIR_DELIM .. name })
			end
		end
	end
	return files
end

local function read_file_content(path)
	local f = io.open(path, "r")
	if not f then return nil end
	local content = f:read("*all")
	f:close()
	return content
end

-- Asset path resolution helper (Part II: Asset Resolution with Namespaces)
local function resolve_asset(asset_str, default_ext)
	if not asset_str or asset_str == "" then
		return nil
	end
	local path = asset_str:match(":(.+)$") or asset_str
	local base_name = path:match("^.*/([^/]+)$") or path:match("^.*\\([^\\]+)$") or path
	if default_ext and not base_name:match("%.%a+$") then
		base_name = base_name .. default_ext
	end
	return base_name
end

-- Runtime Validation helper (Part II: Runtime Validation)
local function validate_serverscript(modname, modpath, script_name, entity_id)
	if script_name then
		local script_path = modpath .. DIR_DELIM .. "ServerSideService" .. DIR_DELIM .. script_name .. ".lua"
		local f = io.open(script_path, "r")
		if f then
			f:close()
		else
			core.log("warning", "[Warning] Object '" .. entity_id .. "' loaded without AI (ServerScript '" .. script_name .. "' not found).")
		end
	end
end

local function validate_clientscript(modname, modpath, script_name, entity_id)
	if script_name then
		local script_path = modpath .. DIR_DELIM .. "ClientSideService" .. DIR_DELIM .. script_name .. ".lua"
		local f = io.open(script_path, "r")
		if f then
			f:close()
		else
			core.log("warning", "[Warning] Object '" .. entity_id .. "' loaded without Client Logic (ClientScript '" .. script_name .. "' not found).")
		end
	end
end

local function validate_asset(modpath, asset_path, sub_folder, entity_id)
	if asset_path and asset_path ~= "" then
		local clean_path = asset_path:match(":(.+)$") or asset_path
		local full_asset_path = modpath .. DIR_DELIM .. "Assets" .. DIR_DELIM .. sub_folder .. DIR_DELIM .. clean_path
		local f = io.open(full_asset_path, "r")
		if not f then
			f = io.open(full_asset_path .. ".png", "r") or io.open(full_asset_path .. ".gltf", "r") or io.open(full_asset_path .. ".glb", "r") or io.open(full_asset_path .. ".obj", "r")
		end

		if f then
			f:close()
		else
			core.log("warning", "[Warning] Asset '" .. asset_path .. "' referenced by '" .. entity_id .. "' was not found in " .. sub_folder .. ".")
		end
	end
end

-- Script loader and executor for Service files
local function execute_service_script(modname, script_name, event, pos, player, object)
	local modpath = core.get_modpath(modname)
	if not modpath then return end
	local script_path = modpath .. DIR_DELIM .. "ServerSideService" .. DIR_DELIM .. script_name .. ".lua"
	local f = loadfile(script_path)
	if f then
		local env = setmetatable({
			pos = pos,
			player = player,
			object = object,
			event = event,
		}, { __index = _G })
		setfenv(f, env)
		local ok, err = pcall(f)
		if ok and type(env[event]) == "function" then
			pcall(env[event], pos, player, object)
		end
	end
end

-- Built-in Rig APIs
local Rig = {}
Rig.__index = Rig

function Rig.new(data, object)
	local self = setmetatable({}, Rig)
	self.data = data
	self.object = object

	-- Auto-apply scale & default transforms
	if object and data then
		local props = object:get_properties() or {}
		if data.Scale then
			props.visual_scale = {x = data.Scale, y = data.Scale, z = data.Scale}
		end
		if data.Collision then
			if data.Collision.Shape == "Capsule" then
				local r = data.Collision.Radius or 0.3
				local h = data.Collision.Height or 1.8
				props.collisionbox = {-r, 0, -r, r, h, r}
			end
		end
		object:set_properties(props)

		if data.Transforms then
			for bone_name, trans in pairs(data.Transforms) do
				if trans.Position then
					self:SetBonePosition(bone_name, trans.Position)
				end
				if trans.Rotation then
					self:SetBoneRotation(bone_name, trans.Rotation)
				end
				if trans.Scale then
					self:SetBoneScale(bone_name, trans.Scale)
				end
			end
		end
	end

	return self
end

function Rig:SetModel(model)
	if self.object then
		local props = self.object:get_properties() or {}
		props.mesh = resolve_asset(model, ".gltf")
		self.object:set_properties(props)
	end
end

function Rig:GetModel()
	if self.object then
		local props = self.object:get_properties()
		return props and props.mesh
	end
	return self.data and resolve_asset(self.data.Model, ".gltf")
end

function Rig:SetCollisionShape(shape)
	if self.object then
		local props = self.object:get_properties() or {}
		if shape == "Capsule" then
			props.collisionbox = {-0.3, 0, -0.3, 0.3, 1.8, 0.3}
		else
			props.collisionbox = {-0.5, 0, -0.5, 0.5, 1.0, 0.5}
		end
		self.object:set_properties(props)
	end
end

function Rig:GetCollisionShape()
	if self.object then
		local props = self.object:get_properties()
		return props and props.collisionbox
	end
	return self.data and self.data.CollisionShape
end

function Rig:SetBonePosition(bone, pos, opts)
	if self.object and self.object.set_bone_position then
		self.object:set_bone_position(bone, pos, opts)
	end
end

function Rig:SetBoneRotation(bone, rot, opts)
	if self.object and self.object.set_bone_rotation then
		self.object:set_bone_rotation(bone, rot, opts)
	end
end

function Rig:SetBoneScale(bone, scale, opts)
	if self.object and self.object.set_bone_scale then
		self.object:set_bone_scale(bone, scale, opts)
	end
end

function Rig:GetBone(bone)
	if self.object and self.object.get_bone_position then
		return self.object:get_bone_position(bone)
	end
	return nil
end

function Rig:HideBone(bone)
	if self.object and self.object.set_part_visible then
		self.object:set_part_visible(bone, false)
	end
end

function Rig:ShowBone(bone)
	if self.object and self.object.set_part_visible then
		self.object:set_part_visible(bone, true)
	end
end

function Rig:AddAttachment(bone, model, texture)
end

function Rig:RemoveAttachment(bone)
end

function Rig:ResetBone(bone)
	if self.object and self.object.set_bone_position then
		self.object:set_bone_position(bone, {x=0, y=0, z=0})
		self.object:set_bone_rotation(bone, {x=0, y=0, z=0})
		self.object:set_bone_scale(bone, {x=1, y=1, z=1})
	end
end

function Rig:ResetPose()
	if self.object and self.data and self.data.Bones then
		for _, bone in ipairs(self.data.Bones) do
			self:ResetBone(bone)
		end
	end
end

core.rigs = {}

function core.rigs.get(id, object)
	local data = registered_rigs[id]
	if data then
		return Rig.new(data, object)
	end
	return nil
end

-- Core Registration functions
local function register_blocks(modname, modpath)
	local files = scan_workspace_dir(modpath, "Blocks", ".block")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -7)
			local id = modname .. ":" .. name

			local def = {
				description = data.DisplayName or name,
				drawtype = "normal",
			}

			if data.Texture then
				validate_asset(modpath, data.Texture, "Textures", id)
				def.tiles = { resolve_asset(data.Texture, ".png") }
			end

			if data.Model then
				validate_asset(modpath, data.Model, "Models", id)
				def.drawtype = "mesh"
				def.mesh = resolve_asset(data.Model, ".obj")
			end

			if data.Physics then
				if data.Physics.Collision == "Box" then
					def.drawtype = "normal"
				end
			end

			if data.Interaction then
				def.walkable = (data.Interaction.Walkable ~= false)
				def.pointable = (data.Interaction.Breakable ~= false)
				def.diggable = (data.Interaction.Breakable ~= false)
			end

			-- Dynamic Event script callbacks (ServerScript)
			if data.ServerScript then
				validate_serverscript(modname, modpath, data.ServerScript, id)
				local s_name = data.ServerScript
				def.on_construct = function(pos)
					execute_service_script(modname, s_name, "OnTouch", pos)
				end
				def.on_punch = function(pos, node, puncher)
					execute_service_script(modname, s_name, "OnPunch", pos, puncher)
				end
				def.on_rightclick = function(pos, node, clicker)
					execute_service_script(modname, s_name, "OnInteract", pos, clicker)
				end
				def.after_place_node = function(pos, placer, itemstack, pointed_thing)
					execute_service_script(modname, s_name, "OnPlace", pos, placer)
				end
				def.after_dig_node = function(pos, oldnode, oldmetadata, digger)
					execute_service_script(modname, s_name, "OnBreak", pos, digger)
				end
			end

			core.register_node(id, def)
		end
	end
end

local function register_items(modname, modpath)
	local files = scan_workspace_dir(modpath, "Items", ".item")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -6)
			local id = modname .. ":" .. name

			if data.ServerScript then
				validate_serverscript(modname, modpath, data.ServerScript, id)
			end

			if data.Texture then
				validate_asset(modpath, data.Texture, "Textures", id)
			end

			-- If item specifies weapon/tool properties, register as a tool
			if data.Damage or data.Durability then
				local def = {
					description = data.DisplayName or name,
					inventory_image = resolve_asset(data.Texture, ".png") or (name .. ".png"),
					wield_image = resolve_asset(data.Texture, ".png"),
					tool_capabilities = {
						full_punch_interval = 1.0,
						max_drop_level = 1,
						groupcaps = {
							fleshy = {times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=data.Durability or 100, maxlevel=1},
						},
						damage_groups = {fleshy = data.Damage or 5},
					}
				}
				core.register_tool(id, def)
			else
				local def = {
					description = data.DisplayName or name,
					inventory_image = resolve_asset(data.Texture, ".png") or (name .. ".png"),
					wield_image = resolve_asset(data.Texture, ".png"),
				}
				core.register_craftitem(id, def)
			end
		end
	end
end

local function register_characters(modname, modpath)
	local files = scan_workspace_dir(modpath, "Characters", ".character")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -11)
			local id = modname .. ":" .. name

			local hp = data.Gameplay and data.Gameplay.Health or 100

			local def = {
				initial_properties = {
					visual = "mesh",
					mesh = resolve_asset(data.Model, ".gltf") or (name .. ".gltf"),
					textures = { resolve_asset(data.Texture, ".png") or (name .. ".png") },
					hp_max = hp,
					physical = true,
					collisionbox = {-0.3, 0, -0.3, 0.3, 1.8, 0.3},
				},
				on_activate = function(self, staticdata, dtime_s)
					self.object:set_armor_groups({fleshy = 100})
					-- Initialize Rig if specified
					if data.Rig then
						self.rig = core.rigs.get(modname .. ":" .. data.Rig, self.object)
					end
					if data.ServerScript then
						execute_service_script(modname, data.ServerScript, "OnActivate", self.object:get_pos(), nil, self.object)
					end
				end,
				on_step = function(self, dtime)
					if data.ServerScript then
						execute_service_script(modname, data.ServerScript, "OnStep", self.object:get_pos(), nil, self.object)
					end
				end,
			}

			if data.ServerScript then
				validate_serverscript(modname, modpath, data.ServerScript, id)
			end
			if data.ClientScript then
				validate_clientscript(modname, modpath, data.ClientScript, id)
			end

			if data.Model then
				validate_asset(modpath, data.Model, "Models", id)
			end
			if data.Texture then
				validate_asset(modpath, data.Texture, "Textures", id)
			end

			core.register_entity(id, def)
		end
	end
end

local function register_vehicles(modname, modpath)
	local files = scan_workspace_dir(modpath, "Vehicles", ".vehicle")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -9)
			local id = modname .. ":" .. name

			local def = {
				initial_properties = {
					visual = "mesh",
					mesh = resolve_asset(data.Model, ".gltf"),
					textures = { resolve_asset(data.Texture, ".png") or (name .. ".png") },
					physical = true,
					collisionbox = {-0.5, 0, -0.5, 0.5, 1.0, 0.5},
				},
				on_activate = function(self, staticdata, dtime_s)
					self.max_speed = data.MaxSpeed or 80
					self.seats = data.Seats or {}
					self.occupied_seats = {}
				end,
				-- Seat Attachment & Customizable Animations API
				sit = function(self, player, seat_name)
					if not seat_name then
						for name, _ in pairs(self.seats) do
							if not self.occupied_seats[name] then
								seat_name = name
								break
							end
						end
					end

					if not seat_name or not self.seats[seat_name] then
						return false
					end

					local seat_def = self.seats[seat_name]
					local bone = ""
					local animation = "Sit"

					if type(seat_def) == "table" then
						bone = seat_def.Bone or seat_def.bone or ""
						animation = seat_def.Animation or seat_def.animation or "Sit"
					elseif type(seat_def) == "string" then
						bone = seat_def
					end

					player:set_attach(self.object, bone, {x=0, y=0, z=0}, {x=0, y=0, z=0})
					self.occupied_seats[seat_name] = player

					if player.set_animation then
						player:set_animation({clip = animation, loop = true})
					end
					return true
				end,
				unsit = function(self, player)
					for name, occupied_player in pairs(self.occupied_seats) do
						if occupied_player == player then
							player:set_detach()
							self.occupied_seats[name] = nil
							if player.set_animation then
								player:set_animation({clip = "Idle", loop = true})
							end
							return true
						end
					end
					return false
				end,
			}

			if data.ServerScript then
				validate_serverscript(modname, modpath, data.ServerScript, id)
			end
			if data.ClientScript then
				validate_clientscript(modname, modpath, data.ClientScript, id)
			end

			if data.Model then
				validate_asset(modpath, data.Model, "Models", id)
			end
			if data.Texture then
				validate_asset(modpath, data.Texture, "Textures", id)
			end

			core.register_entity(id, def)
		end
	end
end

local function register_pathfinding(modname, modpath)
	local files = scan_workspace_dir(modpath, "Pathfinding", ".pathfinding")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -13)
			local id = modname .. ":" .. name
			registered_pathfindings[id] = data
		end
	end
end

local function register_structures(modname, modpath)
	local files = scan_workspace_dir(modpath, "Structures", ".structure")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_structure(content)
			local name = data.Name or f.name:sub(1, -11)
			local id = modname .. ":" .. name
			registered_structures[id] = data
		end
	end
end

local function register_rigs(modname, modpath)
	local files = scan_workspace_dir(modpath, "Rigs", ".rig")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -5)
			local id = modname .. ":" .. name
			registered_rigs[id] = data

			if data.Model then
				validate_asset(modpath, data.Model, "Models", id)
			end
		end
	end
end

local function register_effects(modname, modpath)
	local files = scan_workspace_dir(modpath, "Effects", ".effect")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Particle or data.Sound or f.name:sub(1, -8)
			local id = modname .. ":" .. name
			registered_effects[id] = data

			if data.Particle then
				validate_asset(modpath, data.Particle, "Textures", id)
			end
		end
	end
end

local function register_lights(modname, modpath)
	local files = scan_workspace_dir(modpath, "Lights", ".light")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = f.name:sub(1, -7)
			local id = modname .. ":" .. name
			registered_lights[id] = data
		end
	end
end

local function register_uis(modname, modpath)
	local files = scan_workspace_dir(modpath, "UI", ".ui")
	for _, f in ipairs(files) do
		local content = read_file_content(f.fullpath)
		if content then
			local data = parse_ini(content)
			local name = data.Name or f.name:sub(1, -4)
			local id = modname .. ":" .. name

			-- Read formspec layout file if specified
			if data.Layout then
				validate_asset(modpath, data.Layout, "UI", id)
				local clean_layout_path = data.Layout:match(":(.+)$") or data.Layout
				local fs_path = modpath .. DIR_DELIM .. "Assets" .. DIR_DELIM .. "UI" .. DIR_DELIM .. clean_layout_path
				if not fs_path:match("%.formspec$") then
					fs_path = fs_path .. ".formspec"
				end
				local fs_content = read_file_content(fs_path)
				if fs_content then
					-- Pre-resolve namespaced asset paths inside the formspec string
					fs_content = fs_content:gsub("([%w_%.]+):Textures/([%w_%.]+)", function(pkg, img)
						return resolve_asset(pkg .. ":Textures/" .. img, ".png")
					end)
					data.compiled_formspec = fs_content
				end
			end

			registered_uis[id] = data
		end
	end
end

-- Scan and Load all package definitions when mods have loaded
core.register_on_mods_loaded(function()
	local loaded_mods = core.get_modnames()
	for _, modname in ipairs(loaded_mods) do
		local modpath = core.get_modpath(modname)
		if modpath then
			register_blocks(modname, modpath)
			register_items(modname, modpath)
			register_characters(modname, modpath)
			register_vehicles(modname, modpath)
			register_pathfinding(modname, modpath)
			register_structures(modname, modpath)
			register_rigs(modname, modpath)
			register_effects(modname, modpath)
			register_lights(modname, modpath)
			register_uis(modname, modpath)
		end
	end
end)

-- Built-in Structure APIs
local Structure = {}
Structure.__index = Structure

function Structure.new(data)
	local self = setmetatable({}, Structure)
	self.data = data
	self.rotation = 0
	self.mirrored = false
	return self
end

function Structure:Place(position)
	if not self.data or not self.data.layers then return false end
	local palette = self.data.Palette or {}

	for y_key, layer in pairs(self.data.layers) do
		local y_offset = tonumber(y_key:match("Y(%d+)")) or 0
		local lines = {}
		for line in layer:gmatch("[^\r\n]+") do
			line = trim(line)
			if line ~= "" then
				table.insert(lines, line)
			end
		end

		for z_offset, line in ipairs(lines) do
			local x_idx = 1
			for char in line:gmatch("%S+") do
				local block_id = tonumber(char)
				if block_id then
					local block_name = palette[block_id]
					if block_name and block_name ~= "Air" then
						local pos = {
							x = position.x + (x_idx - 1),
							y = position.y + y_offset,
							z = position.z + (z_offset - 1)
						}
						core.set_node(pos, { name = block_name })
					end
				end
				x_idx = x_idx + 1
			end
		end
	end
	return true
end

function Structure:Rotate(angle)
	self.rotation = (self.rotation + angle) % 360
	return self
end

function Structure:Mirror()
	self.mirrored = not self.mirrored
	return self
end

function Structure:GetMarker(name)
	if self.data and self.data.Markers then
		return self.data.Markers[name]
	end
	return nil
end

function Structure:GetSize()
	return self.data and self.data.Size or {x=1, y=1, z=1}
end

function Structure:Clone()
	local cloned = Structure.new(self.data)
	cloned.rotation = self.rotation
	cloned.mirrored = self.mirrored
	return cloned
end

function Structure:Destroy()
	self.data = nil
end

core.structures = {}

function core.structures.get(id)
	local data = registered_structures[id]
	if data then
		return Structure.new(data)
	end
	return nil
end

-- Built-in Effects Engine API
core.effects = {}
function core.effects.spawn(id, pos)
	local data = registered_effects[id]
	if data then
		if data.Sound then
			core.sound_play(data.Sound, { pos = pos })
		end
		if data.Particle then
			core.add_particlespawner({
				amount = 20,
				time = data.Duration or 1,
				minpos = pos,
				maxpos = pos,
				texture = resolve_asset(data.Particle, ".png") or "smoke_puff.png",
			})
		end
	end
end

-- Built-in UI Declarative Formspecs Engine API
core.ui = {}
function core.ui.show(player_name, ui_id, custom_substitutions)
	local data = registered_uis[ui_id]
	if data and data.compiled_formspec then
		local formspec = data.compiled_formspec
		if custom_substitutions then
			for k, v in pairs(custom_substitutions) do
				formspec = formspec:gsub("%$" .. k, tostring(v))
			end
		end
		core.show_formspec(player_name, ui_id, formspec)
		return true
	end
	return false
end

function core.ui.hide(player_name, ui_id)
	core.show_formspec(player_name, ui_id, "")
end
