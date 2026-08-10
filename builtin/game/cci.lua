-- CCI UI System for Luanti
-- Implements retained-mode 2D UI for modders

rawset(_G, "ui", rawget(_G, "ui") or {})

-- Register style
function ui.style(style_name, def)
	if not style_name or type(def) ~= "table" then
		core.log("error", "[CCI] Invalid style registration for '" .. tostring(style_name) .. "'")
		return false
	end
	core.cci_register_style(style_name, def)
	return true
end

-- Create instance
function ui.create(instance_name, style_name, properties)
	if not instance_name or not style_name or type(properties) ~= "table" then
		core.log("error", "[CCI] Invalid instance creation for '" .. tostring(instance_name) .. "'")
		return false
	end

	-- Extract player name if object is passed
	local player_name = ""
	if properties.player then
		if type(properties.player) == "userdata" and properties.player.get_player_name then
			player_name = properties.player:get_player_name()
		elseif type(properties.player) == "string" then
			player_name = properties.player
		end
	end

	-- Clone properties to avoid modifying original
	local props = {}
	for k, v in pairs(properties) do
		props[k] = v
	end
	props.player = player_name

	core.cci_create_instance(instance_name, style_name, props)
	return true
end

-- Destroy instance
function ui.destroy(instance_name, player)
	if not instance_name then
		return false
	end

	local player_name = ""
	if player then
		if type(player) == "userdata" and player.get_player_name then
			player_name = player:get_player_name()
		elseif type(player) == "string" then
			player_name = player
		end
	end

	core.cci_destroy_instance(instance_name, player_name)
	return true
end

-- Alias delete to destroy for ease of use
ui.delete = ui.destroy
