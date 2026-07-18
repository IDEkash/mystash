-- Minetek Lua Animation Engine (MLAE)
-- Complete Lua-based Animation System

local mlae = {}
core.animation = mlae

mlae.animations = {}
mlae.easings = {}
local easings = mlae.easings

-- Standard Easing Equations (Mapping [0, 1] to [0, 1])
easings.linear = function(t) return t end
easings.smoothstep = function(t) return t * t * (3 - 2 * t) end

-- Sine
easings.sine_in = function(t) return 1 - math.cos(t * math.pi / 2) end
easings.sine_out = function(t) return math.sin(t * math.pi / 2) end
easings.sine_in_out = function(t) return -(math.cos(math.pi * t) - 1) / 2 end
easings.sine = easings.sine_in_out

-- Quad
easings.quad_in = function(t) return t * t end
easings.quad_out = function(t) return 1 - (1 - t) * (1 - t) end
easings.quad_in_out = function(t)
	return t < 0.5 and 2 * t * t or 1 - (-2 * t + 2)^2 / 2
end
easings.quad = easings.quad_in_out

-- Cubic
easings.cubic_in = function(t) return t * t * t end
easings.cubic_out = function(t) return 1 - (1 - t)^3 end
easings.cubic_in_out = function(t)
	return t < 0.5 and 4 * t * t * t or 1 - (-2 * t + 2)^3 / 2
end
easings.cubic = easings.cubic_in_out

-- Quart
easings.quart_in = function(t) return t^4 end
easings.quart_out = function(t) return 1 - (1 - t)^4 end
easings.quart_in_out = function(t)
	return t < 0.5 and 8 * t^4 or 1 - (-2 * t + 2)^4 / 2
end
easings.quart = easings.quart_in_out

-- Quint
easings.quint_in = function(t) return t^5 end
easings.quint_out = function(t) return 1 - (1 - t)^5 end
easings.quint_in_out = function(t)
	return t < 0.5 and 16 * t^5 or 1 - (-2 * t + 2)^5 / 2
end
easings.quint = easings.quint_in_out

-- Expo
easings.expo_in = function(t) return t == 0 and 0 or 2^(10 * t - 10) end
easings.expo_out = function(t) return t == 1 and 1 or 1 - 2^(-10 * t) end
easings.expo_in_out = function(t)
	if t == 0 then return 0 end
	if t == 1 then return 1 end
	return t < 0.5 and 2^(20 * t - 10) / 2 or (2 - 2^(-20 * t + 10)) / 2
end
easings.expo = easings.expo_in_out

-- Circular
easings.circular_in = function(t) return 1 - math.sqrt(1 - t * t) end
easings.circular_out = function(t) return math.sqrt(1 - (t - 1)^2) end
easings.circular_in_out = function(t)
	return t < 0.5 and (1 - math.sqrt(1 - 4 * t * t)) / 2 or (math.sqrt(1 - (-2 * t + 2)^2) + 1) / 2
end
easings.circular = easings.circular_in_out

-- Back
local c1 = 1.70158
local c3 = c1 + 1
easings.back_in = function(t) return c3 * t^3 - c1 * t^2 end
easings.back_out = function(t) return 1 + c3 * (t - 1)^3 + c1 * (t - 1)^2 end
easings.back_in_out = function(t)
	local c2 = c1 * 1.525
	return t < 0.5 and ( (2 * t)^2 * ( (c2 + 1) * 2 * t - c2 ) ) / 2 or ( (2 * t - 2)^2 * ( (c2 + 1) * (t * 2 - 2) + c2 ) + 2 ) / 2
end
easings.back = easings.back_in_out

-- Bounce
local function bounce_out(t)
	local n1 = 7.5625
	local d1 = 2.75
	if t < 1 / d1 then
		return n1 * t * t
	elseif t < 2 / d1 then
		t = t - 1.5 / d1
		return n1 * t * t + 0.75
	elseif t < 2.5 / d1 then
		t = t - 2.25 / d1
		return n1 * t * t + 0.9375
	else
		t = t - 2.625 / d1
		return n1 * t * t + 0.984375
	end
end
easings.bounce_out = bounce_out
easings.bounce_in = function(t) return 1 - bounce_out(1 - t) end
easings.bounce_in_out = function(t)
	return t < 0.5 and (1 - bounce_out(1 - 2 * t)) / 2 or (1 + bounce_out(2 * t - 1)) / 2
end
easings.bounce = easings.bounce_in_out

-- Elastic
easings.elastic_in = function(t)
	if t == 0 then return 0 end
	if t == 1 then return 1 end
	return -2^(10 * t - 10) * math.sin((t * 10 - 10.75) * ((2 * math.pi) / 3))
end
easings.elastic_out = function(t)
	if t == 0 then return 0 end
	if t == 1 then return 1 end
	return 2^(-10 * t) * math.sin((t * 10 - 0.75) * ((2 * math.pi) / 3)) + 1
end
local c5 = (2 * math.pi) / 4.5
easings.elastic_in_out = function(t)
	if t == 0 then return 0 end
	if t == 1 then return 1 end
	if t < 0.5 then
		return -(2^(20 * t - 10) * math.sin((20 * t - 11.125) * c5)) / 2
	else
		return (2^(-20 * t + 10) * math.sin((20 * t - 11.125) * c5)) / 2 + 1
	end
end
easings.elastic = easings.elastic_in_out

-- Direct map aliases to make naming resilient
local function normalize_easing_name(name)
	if not name then return "linear" end
	name = name:lower():gsub("%s+", "_"):gsub("%-+", "_")
	if easings[name] then
		return name
	end
	if name:sub(1, 4) == "ease" and name:sub(5, 5) ~= "_" then
		local rem = name:sub(5)
		if rem == "in" then name = "ease_in"
		elseif rem == "out" then name = "ease_out"
		elseif rem == "inout" then name = "ease_in_out"
		end
	end
	if name == "ease_in" then return "quad_in"
	elseif name == "ease_out" then return "quad_out"
	elseif name == "ease_in_out" then return "quad_in_out"
	end
	return name
end

function mlae.register_easing(name, func)
	assert(type(name) == "string" and type(func) == "function")
	easings[name] = func
end

-- Keyframe binary search with caching (O(1) average lookup)
local function find_keyframes_cached(keyframes, t, cache_index)
	local n = #keyframes
	if n == 0 then return nil, nil, nil end
	if n == 1 then return keyframes[1], nil, 1 end

	if cache_index and cache_index >= 1 and cache_index < n then
		local kf_l = keyframes[cache_index]
		local kf_r = keyframes[cache_index + 1]
		if t >= kf_l.time and t <= kf_r.time then
			return kf_l, kf_r, cache_index
		end
	end

	if t <= keyframes[1].time then
		return keyframes[1], nil, 1
	end
	if t >= keyframes[n].time then
		return keyframes[n], nil, n
	end

	local low = 1
	local high = n
	while low <= high do
		local mid = math.floor((low + high) / 2)
		local kf = keyframes[mid]
		if kf.time == t then
			return kf, keyframes[mid + 1], mid
		elseif kf.time < t then
			if mid < n and keyframes[mid + 1].time > t then
				return kf, keyframes[mid + 1], mid
			end
			low = mid + 1
		else
			if mid > 1 and keyframes[mid - 1].time <= t then
				return keyframes[mid - 1], kf, mid - 1
			end
			high = mid - 1
		end
	end
	return keyframes[1], nil, 1
end

-- Color interpolation support (Hex strings + Table ColorSpecs)
local function parse_hex_color(hex)
	if type(hex) == "table" then
		return { r = hex.r or hex[1] or 255, g = hex.g or hex[2] or 255, b = hex.b or hex[3] or 255, a = hex.a or hex[4] or 255 }
	end
	if type(hex) ~= "string" then
		return { r = 255, g = 255, b = 255, a = 255 }
	end
	hex = hex:gsub("#", "")
	local r, g, b, a = 255, 255, 255, 255
	if #hex == 6 then
		r = tonumber(hex:sub(1, 2), 16) or 255
		g = tonumber(hex:sub(3, 4), 16) or 255
		b = tonumber(hex:sub(5, 6), 16) or 255
	elseif #hex == 8 then
		r = tonumber(hex:sub(1, 2), 16) or 255
		g = tonumber(hex:sub(3, 4), 16) or 255
		b = tonumber(hex:sub(5, 6), 16) or 255
		a = tonumber(hex:sub(7, 8), 16) or 255
	elseif #hex == 3 then
		r = tonumber(hex:sub(1, 1):rep(2), 16) or 255
		g = tonumber(hex:sub(2, 2):rep(2), 16) or 255
		b = tonumber(hex:sub(3, 3):rep(2), 16) or 255
	end
	return { r = r, g = g, b = b, a = a }
end

local function to_hex_color(col)
	return string.format("#%02X%02X%02X%02X", col.r, col.g, col.b, col.a)
end

local function interpolate_color(c1, c2, factor)
	local is_str = (type(c1) == "string" or type(c2) == "string")
	local p1 = parse_hex_color(c1)
	local p2 = parse_hex_color(c2)
	local res = {
		r = math.floor(p1.r + (p2.r - p1.r) * factor + 0.5),
		g = math.floor(p1.g + (p2.g - p1.g) * factor + 0.5),
		b = math.floor(p1.b + (p2.b - p1.b) * factor + 0.5),
		a = math.floor(p1.a + (p2.a - p1.a) * factor + 0.5),
	}
	if is_str then
		return to_hex_color(res)
	else
		return res
	end
end

local function interpolate_val(val_left, val_right, factor, instant)
	if instant or factor >= 1 then
		return val_right
	end
	if factor <= 0 then
		return val_left
	end
	local t1 = type(val_left)
	if t1 == "number" then
		return val_left + (val_right - val_left) * factor
	elseif t1 == "boolean" then
		return factor >= 0.5 and val_right or val_left
	elseif t1 == "table" then
		if val_left.x ~= nil or val_left.y ~= nil or val_left.z ~= nil or val_left[1] ~= nil then
			local lx = val_left.x or val_left[1] or 0
			local ly = val_left.y or val_left[2] or 0
			local lz = val_left.z or val_left[3] or 0
			local rx = val_right.x or val_right[1] or 0
			local ry = val_right.y or val_right[2] or 0
			local rz = val_right.z or val_right[3] or 0
			return {
				x = lx + (rx - lx) * factor,
				y = ly + (ry - ly) * factor,
				z = lz + (rz - lz) * factor
			}
		elseif val_left.r ~= nil or val_left.g ~= nil or val_left.b ~= nil then
			return interpolate_color(val_left, val_right, factor)
		else
			return factor >= 0.5 and val_right or val_left
		end
	elseif t1 == "string" and (val_left:sub(1, 1) == "#" or val_right:sub(1, 1) == "#") then
		return interpolate_color(val_left, val_right, factor)
	end
	return val_left
end

local function evaluate_track(track, t, state_cache, cache_key)
	local cache_idx = state_cache and state_cache[cache_key]
	local kf_l, kf_r, idx = find_keyframes_cached(track, t, cache_idx)
	if state_cache and idx then
		state_cache[cache_key] = idx
	end

	if not kf_l then return nil end
	if not kf_r then
		return kf_l
	end

	local dur = kf_r.time - kf_l.time
	local factor = 0.0
	if dur > 1e-7 then
		factor = (t - kf_l.time) / dur
	end
	factor = math.max(0.0, math.min(1.0, factor))

	local easing_name = normalize_easing_name(kf_r.easing or kf_l.easing)
	local easing_func = easings[easing_name] or easings.linear
	local eased_factor = easing_func(factor)

	local instant = kf_r.instant or kf_l.instant

	local res = {}
	if kf_l.position and kf_r.position then
		res.position = interpolate_val(kf_l.position, kf_r.position, eased_factor, instant)
	elseif kf_l.position then
		res.position = kf_l.position
	end

	if kf_l.rotation and kf_r.rotation then
		res.rotation = interpolate_val(kf_l.rotation, kf_r.rotation, eased_factor, instant)
	elseif kf_l.rotation then
		res.rotation = kf_l.rotation
	end

	if kf_l.scale and kf_r.scale then
		res.scale = interpolate_val(kf_l.scale, kf_r.scale, eased_factor, instant)
	elseif kf_l.scale then
		res.scale = kf_l.scale
	end

	if kf_l.visibility ~= nil and kf_r.visibility ~= nil then
		res.visibility = interpolate_val(kf_l.visibility, kf_r.visibility, eased_factor, instant)
	elseif kf_l.visibility ~= nil then
		res.visibility = kf_l.visibility
	end

	if kf_l.color and kf_r.color then
		res.color = interpolate_val(kf_l.color, kf_r.color, eased_factor, instant)
	elseif kf_l.color then
		res.color = kf_l.color
	end

	if kf_l.glow ~= nil and kf_r.glow ~= nil then
		res.glow = interpolate_val(kf_l.glow, kf_r.glow, eased_factor, instant)
	elseif kf_l.glow ~= nil then
		res.glow = kf_l.glow
	end

	res.weight = interpolate_val(kf_l.weight or 1.0, kf_r.weight or 1.0, eased_factor, instant)
	res.user_data = kf_r.user_data or kf_l.user_data

	return res
end

local function evaluate_curve(curve, t, state_cache, cache_key)
	local cache_idx = state_cache and state_cache[cache_key]
	local kf_l, kf_r, idx = find_keyframes_cached(curve, t, cache_idx)
	if state_cache and idx then
		state_cache[cache_key] = idx
	end

	if not kf_l then return nil end
	if not kf_r then
		return kf_l.value
	end

	local dur = kf_r.time - kf_l.time
	local factor = 0.0
	if dur > 1e-7 then
		factor = (t - kf_l.time) / dur
	end
	factor = math.max(0.0, math.min(1.0, factor))

	local easing_name = normalize_easing_name(kf_r.easing or kf_l.easing)
	local easing_func = easings[easing_name] or easings.linear
	local eased_factor = easing_func(factor)

	local instant = kf_r.instant or kf_l.instant

	return interpolate_val(kf_l.value, kf_r.value, eased_factor, instant)
end

local function blend_transforms(accum, layer, W, additive)
	for part, lt in pairs(layer) do
		local act = accum[part]
		if not act then
			act = { pos = nil, rot = nil, scale = nil, visible = nil, color = nil, glow = nil }
			accum[part] = act
		end

		if additive then
			if lt.position then
				act.pos = act.pos or {x=0, y=0, z=0}
				act.pos.x = act.pos.x + lt.position.x * W
				act.pos.y = act.pos.y + lt.position.y * W
				act.pos.z = act.pos.z + lt.position.z * W
			end
			if lt.rotation then
				act.rot = act.rot or {x=0, y=0, z=0}
				act.rot.x = act.rot.x + lt.rotation.x * W
				act.rot.y = act.rot.y + lt.rotation.y * W
				act.rot.z = act.rot.z + lt.rotation.z * W
			end
			if lt.scale then
				act.scale = act.scale or {x=1, y=1, z=1}
				act.scale.x = act.scale.x * (1 + (lt.scale.x - 1) * W)
				act.scale.y = act.scale.y * (1 + (lt.scale.y - 1) * W)
				act.scale.z = act.scale.z * (1 + (lt.scale.z - 1) * W)
			end
			if lt.visibility ~= nil then
				if act.visible == nil then
					act.visible = lt.visibility
				elseif W >= 0.5 then
					act.visible = lt.visibility
				end
			end
			if lt.color then
				if not act.color then
					act.color = lt.color
				else
					act.color = interpolate_color(act.color, lt.color, W)
				end
			end
			if lt.glow then
				act.glow = act.glow or 0
				act.glow = act.glow + lt.glow * W
			end
		else
			if lt.position then
				if not act.pos then
					act.pos = {x=lt.position.x, y=lt.position.y, z=lt.position.z}
				else
					act.pos.x = act.pos.x + (lt.position.x - act.pos.x) * W
					act.pos.y = act.pos.y + (lt.position.y - act.pos.y) * W
					act.pos.z = act.pos.z + (lt.position.z - act.pos.z) * W
				end
			end
			if lt.rotation then
				if not act.rot then
					act.rot = {x=lt.rotation.x, y=lt.rotation.y, z=lt.rotation.z}
				else
					act.rot.x = act.rot.x + (lt.rotation.x - act.rot.x) * W
					act.rot.y = act.rot.y + (lt.rotation.y - act.rot.y) * W
					act.rot.z = act.rot.z + (lt.rotation.z - act.rot.z) * W
				end
			end
			if lt.scale then
				if not act.scale then
					act.scale = {x=lt.scale.x, y=lt.scale.y, z=lt.scale.z}
				else
					act.scale.x = act.scale.x + (lt.scale.x - act.scale.x) * W
					act.scale.y = act.scale.y + (lt.scale.y - act.scale.y) * W
					act.scale.z = act.scale.z + (lt.scale.z - act.scale.z) * W
				end
			end
			if lt.visibility ~= nil then
				if act.visible == nil then
					act.visible = lt.visibility
				elseif W >= 0.5 then
					act.visible = lt.visibility
				end
			end
			if lt.color then
				if not act.color then
					act.color = lt.color
				else
					act.color = interpolate_color(act.color, lt.color, W)
				end
			end
			if lt.glow ~= nil then
				if not act.glow then
					act.glow = lt.glow
				else
					act.glow = act.glow + (lt.glow - act.glow) * W
				end
			end
		end
	end
end

-- Crossover event/marker checks
local function crossed(prev_frame, cur_frame, marker_frame, loop, range_start, range_end, dir)
	local len = range_end - range_start
	if len <= 0 then
		return false
	end

	if not loop then
		if dir >= 0 then
			return prev_frame < marker_frame and marker_frame <= cur_frame
		end
		return cur_frame <= marker_frame and marker_frame < prev_frame
	end

	if dir >= 0 then
		if cur_frame >= prev_frame then
			return prev_frame < marker_frame and marker_frame <= cur_frame
		end
		return marker_frame > prev_frame or marker_frame <= cur_frame
	end

	if cur_frame <= prev_frame then
		return cur_frame <= marker_frame and marker_frame < prev_frame
	end
	return marker_frame < prev_frame or marker_frame >= cur_frame
end

local function check_events_crossed(instance, prev_t, cur_t, length, loop, dir, callback)
	local events = instance.animation:get_events()
	if #events == 0 then return end
	for _, ev in ipairs(events) do
		local et = ev.time
		if crossed(prev_t, cur_t, et, loop, 0.0, length, dir) then
			callback(ev.name, ev.data, et)
		end
	end
end

local function check_markers_crossed(instance, prev_t, cur_t, length, loop, dir, callback)
	local markers = instance.animation:get_markers()
	if #markers == 0 then return end
	for _, mk in ipairs(markers) do
		local mt = mk.time
		if crossed(prev_t, cur_t, mt, loop, 0.0, length, dir) then
			callback(mk.name, mk.data, mt)
		end
	end
end

-- Apply bone transforms helper
local function apply_bone_transform(object, part_name, t_data, interpolation, absolute)
	local tbl = {}
	if t_data.pos then
		tbl.position = { vec = t_data.pos, absolute = absolute, interpolation = interpolation or 0 }
	end
	if t_data.rot then
		tbl.rotation = { vec = t_data.rot, absolute = absolute, interpolation = interpolation or 0, degrees = true }
	end
	if t_data.scale then
		tbl.scale = { vec = t_data.scale, absolute = absolute, interpolation = interpolation or 0 }
	end
	if t_data.visible ~= nil then
		tbl.visible = t_data.visible
		object:set_part_visible(part_name, t_data.visible)
	end
	if t_data.color then
		tbl.color = t_data.color
	end
	if t_data.glow then
		tbl.glow = t_data.glow
	end

	object:set_bone_override(part_name, tbl)
end

-- Animation Class Definition
local Animation = {}
Animation.__index = Animation

function Animation:new(name)
	local o = {
		name = name,
		length = 1.0,
		tracks = {},
		curves = {},
		markers = {},
		events = {},
		event_callbacks = {},
	}
	return setmetatable(o, Animation)
end

function Animation:set_length(len)
	self.length = tonumber(len) or 1.0
end

function Animation:get_length()
	return self.length
end

function Animation:create_track(part_name)
	if not self.tracks[part_name] then
		self.tracks[part_name] = {}
	end
	return self.tracks[part_name]
end

function Animation:rename_track(old_name, new_name)
	if self.tracks[old_name] then
		self.tracks[new_name] = self.tracks[old_name]
		self.tracks[old_name] = nil
		for _, kf in ipairs(self.tracks[new_name]) do
			kf.part = new_name
		end
		return true
	end
	return false
end

function Animation:get_tracks()
	local list = {}
	for k, _ in pairs(self.tracks) do
		table.insert(list, k)
	end
	table.sort(list)
	return list
end

function Animation:add_keyframe(kf)
	assert(type(kf) == "table")
	local part = kf.part
	if not part then return end

	local track = self:create_track(part)
	local new_kf = {
		time = tonumber(kf.time) or 0.0,
		position = kf.position,
		rotation = kf.rotation,
		scale = kf.scale,
		visibility = kf.visibility,
		color = kf.color,
		glow = kf.glow,
		easing = kf.easing,
		instant = kf.instant,
		weight = kf.weight or 1.0,
		user_data = kf.user_data,
	}

	for i, existing in ipairs(track) do
		if math.abs(existing.time - new_kf.time) < 1e-7 then
			table.remove(track, i)
			break
		end
	end

	table.insert(track, new_kf)
	table.sort(track, function(a, b) return a.time < b.time end)
	return new_kf
end

function Animation:remove_keyframe(part, time)
	local track = self.tracks[part]
	if not track then return false end
	for i, kf in ipairs(track) do
		if math.abs(kf.time - time) < 1e-7 then
			table.remove(track, i)
			return true
		end
	end
	return false
end

function Animation:move_keyframe(part, old_time, new_time)
	local track = self.tracks[part]
	if not track then return false end
	for i, kf in ipairs(track) do
		if math.abs(kf.time - old_time) < 1e-7 then
			kf.time = new_time
			table.sort(track, function(a, b) return a.time < b.time end)
			return true
		end
	end
	return false
end

function Animation:edit_keyframe(part, time, new_data)
	local track = self.tracks[part]
	if not track then return false end
	for i, kf in ipairs(track) do
		if math.abs(kf.time - time) < 1e-7 then
			for k, v in pairs(new_data) do
				if k ~= "time" then
					kf[k] = v
				end
			end
			return true
		end
	end
	return false
end

function Animation:get_keyframes(part)
	local track = self.tracks[part]
	if not track then return {} end
	local copy = {}
	for _, kf in ipairs(track) do
		table.insert(copy, kf)
	end
	return copy
end

function Animation:add_marker(time, name, data)
	local marker = { time = tonumber(time) or 0.0, name = name, data = data }
	table.insert(self.markers, marker)
	table.sort(self.markers, function(a, b) return a.time < b.time end)
	return marker
end

function Animation:get_markers()
	return self.markers
end

function Animation:add_event(time, name, data)
	local event = { time = tonumber(time) or 0.0, name = name, data = data }
	table.insert(self.events, event)
	table.sort(self.events, function(a, b) return a.time < b.time end)
	return event
end

function Animation:get_events()
	return self.events
end

function Animation:on_event(callback)
	assert(type(callback) == "function")
	table.insert(self.event_callbacks, callback)
end

function Animation:register_easing(name, func)
	mlae.register_easing(name, func)
end

function Animation:create_curve(curve_name)
	if not self.curves[curve_name] then
		self.curves[curve_name] = {}
	end
	return self.curves[curve_name]
end

function Animation:add_curve_keyframe(curve_name, time, value, easing, instant)
	local curve = self:create_curve(curve_name)
	local new_kf = {
		time = tonumber(time) or 0.0,
		value = value,
		easing = easing,
		instant = instant,
	}
	for i, existing in ipairs(curve) do
		if math.abs(existing.time - new_kf.time) < 1e-7 then
			table.remove(curve, i)
			break
		end
	end
	table.insert(curve, new_kf)
	table.sort(curve, function(a, b) return a.time < b.time end)
	return new_kf
end

-- Serialization & Stats
function Animation:get_memory_usage()
	local serialized = self:serialize()
	return #serialized
end

function Animation:get_layers()
	-- Animations are independent of layers, but we support the API call
	return {}
end

function Animation:get_statistics()
	local total_keyframes = 0
	for _, track in pairs(self.tracks) do
		total_keyframes = total_keyframes + #track
	end
	for _, track in pairs(self.curves) do
		total_keyframes = total_keyframes + #track
	end
	return {
		tracks = #self:get_tracks(),
		keyframes = total_keyframes,
		length = self.length,
		events = #self.events,
		markers = #self.markers,
	}
end

function Animation:serialize(format)
	local data = {
		name = self.name,
		length = self.length,
		tracks = self.tracks,
		curves = self.curves,
		markers = self.markers,
		events = self.events,
	}
	local json = core.write_json(data)
	if format == "compressed" then
		return core.compress(json, "deflate")
	elseif format == "binary" then
		return core.serialize(data)
	end
	return json
end

function Animation:clone()
	return mlae.deserialize(self:serialize())
end
Animation.copy = Animation.clone
Animation.duplicate = Animation.clone

function Animation:merge(other)
	assert(other and other.tracks)
	for part, track in pairs(other.tracks) do
		local dest = self:create_track(part)
		for _, kf in ipairs(track) do
			self:add_keyframe({
				part = part,
				time = kf.time,
				position = kf.position,
				rotation = kf.rotation,
				scale = kf.scale,
				visibility = kf.visibility,
				color = kf.color,
				glow = kf.glow,
				easing = kf.easing,
				instant = kf.instant,
				weight = kf.weight,
				user_data = kf.user_data,
			})
		end
	end
	for _, m in ipairs(other.markers) do
		self:add_marker(m.time, m.name, m.data)
	end
	for _, e in ipairs(other.events) do
		self:add_event(e.time, e.name, e.data)
	end
end

mlae.Animation = Animation

function mlae.create(name)
	return Animation:new(name)
end

function mlae.deserialize(str)
	local data
	if str:sub(1, 2) == "\x78\x9c" or str:sub(1, 2) == "\x78\x5e" or str:sub(1, 2) == "\x78\x01" then
		local decompressed = core.decompress(str, "deflate")
		data = core.parse_json(decompressed)
	elseif str:sub(1, 4) == "return" or str:sub(1, 1) == "{" then
		if str:sub(1, 1) == "{" then
			data = core.parse_json(str)
		else
			data = core.deserialize(str)
		end
	else
		data = core.parse_json(str)
	end

	assert(data, "failed to deserialize animation")
	local anim = Animation:new(data.name)
	anim.length = data.length or 1.0
	anim.tracks = data.tracks or {}
	anim.curves = data.curves or {}
	anim.markers = data.markers or {}
	anim.events = data.events or {}
	return anim
end


-- Playback Track Instance
local PlaybackInstance = {}
PlaybackInstance.__index = PlaybackInstance

function PlaybackInstance:new(animation, opts)
	opts = opts or {}
	local o = {
		animation = animation,
		time = 0.0,
		speed = opts.speed or 1.0,
		loop = opts.loop ~= false,
		pingpong = opts.pingpong or false,
		playing = true,
		weight = opts.weight or 1.0,
		direction = 1,
		fade_time = opts.fade_time or 0.0,
		fade_timer = opts.fade_time or 0.0,
		fade_type = opts.fade_type, -- "in", "out"
		fade_weight = 1.0,
		cache = {},
	}
	return setmetatable(o, PlaybackInstance)
end


-- Animation Controller Class
local AnimationController = {}
AnimationController.__index = AnimationController

function AnimationController:new(object)
	local o = {
		object = object,
		animations = {},
		layers = {},
		layer_order = {},
		current_state = "Idle",
		time_in_state = 0.0,
		states = {},
		transitions = {},
		last_transition_time = {},
		queue = {},
		procedural_overrides = {},
		curves_values = {},
		absolute_mode = true,
		root_motion_enabled = false,
		root_motion_part = nil,
		last_root_pos = nil,
		movement_drives_animation = false,
		movement_drive_scale = 0.25,
		sleeping = false,
		on_event_callback = nil,
		on_state_change = nil,
		debug_enabled = false,
		statistics = {
			update_cost_us = 0,
			fps_counter = 0,
		}
	}
	local ctrl = setmetatable(o, AnimationController)

	-- Pre-populate standard layers
	ctrl:get_layer("Base", { priority = 0 })
	ctrl:get_layer("Walk", { priority = 10 })
	ctrl:get_layer("UpperBody", { priority = 20 })
	ctrl:get_layer("Face", { priority = 30 })
	ctrl:get_layer("Weapon", { priority = 40 })
	ctrl:get_layer("Additive", { priority = 50, blend_mode = "additive" })

	return ctrl
end

function AnimationController:add_animation(anim_or_name, anim_obj)
	if type(anim_or_name) == "string" then
		assert(anim_obj)
		self.animations[anim_or_name] = anim_obj
	else
		assert(anim_or_name and anim_or_name.name)
		self.animations[anim_or_name.name] = anim_or_name
	end
end

function AnimationController:get_layer(layer_name, def)
	def = def or {}
	if not self.layers[layer_name] then
		local layer = {
			name = layer_name,
			priority = def.priority or 0,
			weight = def.weight or 1.0,
			enabled = def.enabled ~= false,
			mask = def.mask, -- list/table of part names
			blend_mode = def.blend_mode or "override", -- "override" or "additive"
			instances = {},
		}
		self.layers[layer_name] = layer
		table.insert(self.layer_order, layer_name)
		-- Sort layer_order by priority
		table.sort(self.layer_order, function(a, b)
			return (self.layers[a].priority or 0) < (self.layers[b].priority or 0)
		end)
	end
	return self.layers[layer_name]
end

function AnimationController:get_layers()
	local copy = {}
	for _, k in ipairs(self.layer_order) do
		table.insert(copy, self.layers[k])
	end
	return copy
end

function AnimationController:play(anim_name, opts)
	opts = opts or {}
	local anim = self.animations[anim_name]
	if not anim then
		-- fallback to database
		anim = mlae.animations and mlae.animations[anim_name]
	end
	if not anim then
		core.log("warning", "MLAE: Animation not found: " .. tostring(anim_name))
		return nil
	end

	local layer_name = opts.layer or "Base"
	local layer = self:get_layer(layer_name)

	local blend_time = opts.blend_time or 0.1
	if blend_time > 0 then
		for _, inst in ipairs(layer.instances) do
			if inst.animation.name ~= anim_name then
				inst.fade_type = "out"
				inst.fade_time = blend_time
				inst.fade_timer = inst.fade_weight and (inst.fade_weight * blend_time) or blend_time
			end
		end
	else
		layer.instances = {}
	end

	local inst = PlaybackInstance:new(anim, opts)
	if blend_time > 0 then
		inst.fade_type = "in"
		inst.fade_time = blend_time
		inst.fade_timer = 0.0
		inst.fade_weight = 0.0
	else
		inst.fade_weight = 1.0
	end

	table.insert(layer.instances, inst)
	return inst
end

function AnimationController:stop(anim_name, blend_time)
	blend_time = blend_time or 0.1
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				if blend_time > 0 then
					inst.fade_type = "out"
					inst.fade_time = blend_time
					inst.fade_timer = inst.fade_weight and (inst.fade_weight * blend_time) or blend_time
				else
					inst.fade_weight = 0.0
					inst.playing = false
				end
			end
		end
	end
end

function AnimationController:pause(anim_name)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				inst.playing = false
			end
		end
	end
end

function AnimationController:resume(anim_name)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				inst.playing = true
			end
		end
	end
end

function AnimationController:seek(anim_name, time)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				inst.time = time
			end
		end
	end
end

function AnimationController:set_speed(anim_name, speed)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				inst.speed = speed
			end
		end
	end
end

function AnimationController:set_weight(anim_name, weight)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				inst.weight = weight
			end
		end
	end
end

function AnimationController:get_animation_time(anim_name)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if inst.animation.name == anim_name then
				return inst.time
			end
		end
	end
	return nil
end

function AnimationController:get_animation_speed(anim_name)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if inst.animation.name == anim_name then
				return inst.speed
			end
		end
	end
	return nil
end

function AnimationController:is_playing(anim_name)
	for _, layer in pairs(self.layers) do
		for _, inst in ipairs(layer.instances) do
			if not anim_name or inst.animation.name == anim_name then
				if inst.playing and (not inst.fade_type or inst.fade_type ~= "out") then
					return true
				end
			end
		end
	end
	return false
end

function AnimationController:get_current_animation()
	for _, lname in ipairs(self.layer_order) do
		local layer = self.layers[lname]
		if layer and layer.enabled then
			for _, inst in ipairs(layer.instances) do
				if inst.playing then
					return inst.animation.name
				end
			end
		end
	end
	return nil
end

function AnimationController:get_animation_layers()
	local list = {}
	for _, lname in ipairs(self.layer_order) do
		table.insert(list, lname)
	end
	return list
end

-- Queue methods
function AnimationController:enqueue(anim_name, opts)
	table.insert(self.queue, { name = anim_name, opts = opts })
end

function AnimationController:dequeue()
	if #self.queue > 0 then
		return table.remove(self.queue, 1)
	end
	return nil
end

function AnimationController:clear_queue()
	self.queue = {}
end

function AnimationController:peek_queue()
	return self.queue[1]
end

-- State Machine methods
function AnimationController:add_state(name, state_def)
	self.states[name] = state_def
end

function AnimationController:add_transition(from_state, to_state, condition_func, priority, cooldown)
	table.insert(self.transitions, {
		from = from_state,
		to = to_state,
		condition = condition_func,
		priority = priority or 0,
		cooldown = cooldown or 0.0,
	})
	table.sort(self.transitions, function(a, b) return a.priority > b.priority end)
end

function AnimationController:set_state(name)
	if self.current_state == name then return end
	local old_state = self.current_state
	self.current_state = name
	self.time_in_state = 0.0

	if self.on_state_change then
		self.on_state_change(old_state, name)
	end

	local state_def = self.states[name]
	if state_def then
		local anim_name = state_def.animation
		if anim_name then
			self:play(anim_name, {
				blend_time = state_def.blend_time,
				speed = state_def.speed,
				loop = state_def.loop,
				weight = state_def.weight,
				layer = state_def.layer or "Base",
			})
		end
	end
end

function AnimationController:get_state()
	return self.current_state
end

-- Sleep / Optimize
function AnimationController:sleep()
	self.sleeping = true
end

function AnimationController:wakeup()
	self.sleeping = false
end

function AnimationController:is_sleeping()
	return self.sleeping
end

function AnimationController:set_procedural_override(part_name, callback)
	self.procedural_overrides[part_name] = callback
end

function AnimationController:get_curve_value(curve_name)
	return self.curves_values[curve_name]
end

function AnimationController:on_event(callback)
	self.on_event_callback = callback
end

function AnimationController:set_root_motion_enabled(enabled, root_part_name)
	self.root_motion_enabled = enabled
	self.root_motion_part = root_part_name
	self.last_root_pos = nil
end

function AnimationController:set_debug(enabled)
	self.debug_enabled = enabled
end

function AnimationController:get_statistics()
	local active_layers = 0
	local active_anims = 0
	for _, layer in pairs(self.layers) do
		if layer.enabled and layer.weight > 0 then
			active_layers = active_layers + 1
			for _, inst in ipairs(layer.instances) do
				if inst.playing then
					active_anims = active_anims + 1
				end
			end
		end
	end
	return {
		active_layers = active_layers,
		active_animations = active_anims,
		queue_size = #self.queue,
		update_cost_us = self.statistics.update_cost_us,
		fps = self.statistics.fps_counter,
	}
end

function AnimationController:update(dtime)
	if self.sleeping then return end
	if not self.object or not self.object:is_valid() then
		mlae.active_controllers[self.object] = nil
		return
	end

	local start_time = core.get_us_time and core.get_us_time() or 0

	-- 1. State transitions evaluation
	self.time_in_state = self.time_in_state + dtime
	local current_s = self.current_state
	if current_s then
		for _, tr in ipairs(self.transitions) do
			if tr.from == "*" or tr.from == current_s then
				local last_t = self.last_transition_time[tr.from .. "->" .. tr.to] or 0.0
				local now = self.time_in_state
				if (now - last_t) >= tr.cooldown then
					local cond = tr.condition
					if not cond or cond(self, self.object) then
						self.last_transition_time[tr.from .. "->" .. tr.to] = now
						self:set_state(tr.to)
						break
					end
				end
			end
		end
	end

	-- 2. Movement drives animation speed
	if self.movement_drives_animation then
		local vel = self.object:get_velocity()
		if vel then
			local spd = math.sqrt(vel.x^2 + vel.z^2)
			for _, layer in pairs(self.layers) do
				for _, inst in ipairs(layer.instances) do
					if inst.animation.name == "walk" or inst.animation.name == "run" then
						inst.speed = spd * (self.movement_drive_scale or 0.25)
					end
				end
			end
		end
	end

	-- 3. Layer Evaluation & Mixing
	local accumulated = {}
	local curve_values = {}

	for _, lname in ipairs(self.layer_order) do
		local layer = self.layers[lname]
		if layer and layer.enabled and layer.weight > 0 then
			local layer_accum = {}
			local layer_curves = {}

			local active_instances = {}
			for _, inst in ipairs(layer.instances) do
				if inst.playing then
					if inst.fade_type == "in" then
						inst.fade_timer = inst.fade_timer + dtime
						if inst.fade_timer >= inst.fade_time then
							inst.fade_weight = 1.0
							inst.fade_type = nil
						else
							inst.fade_weight = inst.fade_timer / inst.fade_time
						end
					elseif inst.fade_type == "out" then
						inst.fade_timer = inst.fade_timer - dtime
						if inst.fade_timer <= 0 then
							inst.fade_weight = 0.0
							inst.playing = false
						else
							inst.fade_weight = inst.fade_timer / inst.fade_time
						end
					end

					if inst.playing then
						table.insert(active_instances, inst)

						local prev_t = inst.time
						local delta = dtime * inst.speed * inst.direction
						inst.time = inst.time + delta
						local length = inst.animation:get_length() or 0

						local finished = false
						if length > 0 then
							if inst.pingpong then
								if inst.direction > 0 and inst.time >= length then
									inst.time = length - (inst.time - length)
									inst.direction = -1
								elseif inst.direction < 0 and inst.time <= 0 then
									inst.time = -inst.time
									inst.direction = 1
								end
							elseif inst.loop then
								if inst.time >= length then
									inst.time = inst.time % length
								elseif inst.time < 0 then
									inst.time = length - (-inst.time % length)
								end
							else
								if inst.time >= length then
									inst.time = length
									inst.playing = false
									finished = true
								elseif inst.time <= 0 then
									inst.time = 0
									inst.playing = false
									finished = true
								end
							end
						end

						local function fire_event(ev_name, ev_data, ev_time)
							for _, cb in ipairs(inst.animation.event_callbacks) do
								cb(ev_name, ev_data, ev_time)
							end
							if self.on_event_callback then
								self.on_event_callback(inst.animation.name, ev_name, ev_data, ev_time)
							end
						end

						if length > 0 then
							check_events_crossed(inst, prev_t, inst.time, length, inst.loop, inst.direction, fire_event)
							check_markers_crossed(inst, prev_t, inst.time, length, inst.loop, inst.direction, fire_event)
						end

						if finished and lname == "Base" and #self.queue > 0 then
							local next_anim = self:dequeue()
							if next_anim then
								self:play(next_anim.name, next_anim.opts)
							end
						end

						local eff_weight = inst.weight * (inst.fade_weight or 1.0)
						if eff_weight > 0 then
							local inst_transforms = {}
							for part, track in pairs(inst.animation.tracks) do
								local masked = true
								if layer.mask then
									masked = false
									for _, mpart in ipairs(layer.mask) do
										if mpart == part then
											masked = true
											break
										end
									end
								end

								if masked then
									-- Optimize: evaluate track, skipping if invisible
									local t_data = evaluate_track(track, inst.time, inst.cache, "track:" .. part)
									if t_data then
										inst_transforms[part] = t_data
									end
								end
							end

							for curve_name, curve in pairs(inst.animation.curves) do
								local val = evaluate_curve(curve, inst.time, inst.cache, "curve:" .. curve_name)
								if val ~= nil then
									layer_curves[curve_name] = layer_curves[curve_name] or {}
									table.insert(layer_curves[curve_name], { val = val, w = eff_weight })
								end
							end

							blend_transforms(layer_accum, inst_transforms, eff_weight, false)
						end
					end
				end
			end
			layer.instances = active_instances

			local layer_eff_weight = layer.weight
			if layer_eff_weight > 0 then
				local additive = (layer.blend_mode == "additive")
				blend_transforms(accumulated, layer_accum, layer_eff_weight, additive)

				for cname, entries in pairs(layer_curves) do
					local sum_val = 0
					local sum_w = 0
					for _, entry in ipairs(entries) do
						sum_val = sum_val + entry.val * entry.w
						sum_w = sum_w + entry.w
					end
					if sum_w > 0 then
						local blended_layer_curve_val = sum_val / sum_w
						if not curve_values[cname] then
							curve_values[cname] = blended_layer_curve_val
						else
							curve_values[cname] = curve_values[cname] + (blended_layer_curve_val - curve_values[cname]) * layer_eff_weight
						end
					end
				end
			end
		end
	end

	self.curves_values = curve_values

	-- 4. Root motion evaluation
	if self.root_motion_enabled and self.root_motion_part then
		local current_root_pos = accumulated[self.root_motion_part] and accumulated[self.root_motion_part].pos
		if current_root_pos and self.last_root_pos then
			local dx = current_root_pos.x - self.last_root_pos.x
			local dy = current_root_pos.y - self.last_root_pos.y
			local dz = current_root_pos.z - self.last_root_pos.z
			if self.object and self.object:is_valid() then
				local pos = self.object:get_pos()
				if pos then
					self.object:set_pos({ x = pos.x + dx, y = pos.y + dy, z = pos.z + dz })
				end
			end
		end
		if current_root_pos then
			self.last_root_pos = { x = current_root_pos.x, y = current_root_pos.y, z = current_root_pos.z }
		end
	end

	-- 5. Procedural Overrides
	if self.procedural_overrides then
		for part, callback in pairs(self.procedural_overrides) do
			local act = accumulated[part] or { pos=nil, rot=nil, scale=nil, visible=nil, color=nil, glow=nil }
			local overriden = callback(part, act, self.object)
			if overriden then
				accumulated[part] = overriden
			end
		end
	end

	-- 6. Apply final accumulated transforms to bone overrides of object
	local interpolation = 0.0
	local absolute = (self.absolute_mode ~= false)
	for part, t_data in pairs(accumulated) do
		apply_bone_transform(self.object, part, t_data, interpolation, absolute)
	end

	local end_time = core.get_us_time and core.get_us_time() or 0
	local cost = end_time - start_time
	self.statistics.update_cost_us = cost
	self.statistics.fps_counter = (self.statistics.fps_counter or 0) + 1

	if self.debug_enabled then
		core.log("action", string.format("MLAE Debug: cost=%dus queue=%d active_anims=%d curves=%d",
			cost, #self.queue, self:get_statistics().active_animations, #self.layer_order))
	end
end

mlae.AnimationController = AnimationController

function mlae.create_controller(object)
	local ctrl = AnimationController:new(object)
	mlae.active_controllers[object] = ctrl
	return ctrl
end
mlae.controller = mlae.create_controller

-- Global active controllers registry using weak keys to prevent memory leaks
mlae.active_controllers = setmetatable({}, { __mode = "k" })

-- Global step registration to automatically update active controllers
if INIT == "game" and core.register_globalstep then
	core.register_globalstep(function(dtime)
		for obj, ctrl in pairs(mlae.active_controllers) do
			if not obj or not obj.is_valid or not obj:is_valid() then
				mlae.active_controllers[obj] = nil
			else
				ctrl:update(dtime)
			end
		end
	end)
end

-- Monkeypatch ObjectRef with MLAE Runtime Information methods
local ObjectRef_meta = nil
if debug and type(debug.getregistry) == "function" then
	local reg = debug.getregistry()
	if type(reg) == "table" then
		ObjectRef_meta = reg.ObjectRef
	end
end

local methods = ObjectRef_meta and ObjectRef_meta.__index
if type(methods) == "table" then
	function methods:get_animation_state()
		local ctrl = mlae.active_controllers[self]
		return ctrl and ctrl:get_state()
	end
	function methods:get_current_animation()
		local ctrl = mlae.active_controllers[self]
		return ctrl and ctrl:get_current_animation()
	end
	function methods:get_animation_time()
		local ctrl = mlae.active_controllers[self]
		return ctrl and ctrl:get_animation_time()
	end
	function methods:get_animation_speed()
		local ctrl = mlae.active_controllers[self]
		return ctrl and ctrl:get_animation_speed()
	end
	function methods:get_animation_layers()
		local ctrl = mlae.active_controllers[self]
		return ctrl and ctrl:get_animation_layers()
	end
	function methods:is_animation_playing()
		local ctrl = mlae.active_controllers[self]
		return ctrl and ctrl:is_animation_playing() or false
	end
end
