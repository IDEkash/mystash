-- Minetek Lua Animation Engine (MLAE)
-- Complete Lua-based Animation System with Advanced Editor Features

local mlae = {}
core.animation = mlae

mlae.animations = {}
mlae.easings = {}
local easings = mlae.easings

-- Standard & Advanced Easing/Interpolation Equations
easings.linear = function(t) return t end
easings.constant = function(t) return t < 1.0 and 0.0 or 1.0 end
easings.step = function(t) return t < 0.5 and 0.0 or 1.0 end
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

-- Interpolation Curves (Bezier, Hermite, Catmull-Rom, TCB)
local function bezier_interpolate(p0, p1, p2, p3, t)
	local u = 1 - t
	local tt = t * t
	local uu = u * u
	local uuu = uu * u
	local ttt = tt * t
	return uuu * p0 + 3 * uu * t * p1 + 3 * u * tt * p2 + ttt * p3
end

local function hermite_interpolate(p0, t0, p1, t1, t)
	local t2 = t * t
	local t3 = t2 * t
	local h00 = 2 * t3 - 3 * t2 + 1
	local h10 = t3 - 2 * t2 + t
	local h01 = -2 * t3 + 3 * t2
	local h11 = t3 - t2
	return h00 * p0 + h10 * t0 + h01 * p1 + h11 * t1
end

local function catmull_rom_interpolate(p0, p1, p2, p3, t)
	return 0.5 * (
		(2 * p1) +
		(-p0 + p2) * t +
		(2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
		(-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t
	)
end

-- TCB (Tension, Continuity, Bias)
local function tcb_interpolate(p0, p1, p2, p3, tension, continuity, bias, t)
	tension = tension or 0.0
	continuity = continuity or 0.0
	bias = bias or 0.0
	local s0 = (1 - tension) * (1 + continuity) * (1 + bias) / 2
	local s1 = (1 - tension) * (1 - continuity) * (1 - bias) / 2
	local s2 = (1 - tension) * (1 - continuity) * (1 + bias) / 2
	local s3 = (1 - tension) * (1 + continuity) * (1 - bias) / 2
	local m0 = s0 * (p1 - p0) + s1 * (p2 - p1)
	local m1 = s2 * (p2 - p1) + s3 * (p3 - p2)
	return hermite_interpolate(p1, m0, p2, m1, t)
end

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

-- Stable Keyframe IDs & Objects
local Keyframe = {}
Keyframe.__index = Keyframe

function Keyframe:new(data)
	local o = {
		id = data.id,
		time = tonumber(data.time) or 0.0,
		position = data.position,
		rotation = data.rotation,
		scale = data.scale,
		visibility = data.visibility,
		color = data.color,
		glow = data.glow,
		easing = data.easing,
		instant = data.instant,
		weight = data.weight or 1.0,
		user_data = data.user_data,
		metadata = data.metadata or {},
		tags = data.tags or {},
		created_at = data.created_at or os.time(),
		modified_at = data.modified_at or os.time(),
		left_handle = data.left_handle,
		right_handle = data.right_handle,
		handle_mode = data.handle_mode, -- "free", "aligned", "aligned_free"
		tension = data.tension,
		continuity = data.continuity,
		bias = data.bias,
	}
	return setmetatable(o, Keyframe)
end

function Keyframe:set_metadata(k, v)
	self.metadata[k] = v
	self.modified_at = os.time()
end

function Keyframe:get_metadata(k)
	return self.metadata[k]
end

-- First-Class Track Object
local Track = {}

function Track:new(name)
	local o = {
		name = name,
		keyframes = {}, -- list of Keyframe objects
		metadata = {},
		color = nil,
		icon = nil,
		visible = true,
		locked = false,
		muted = false,
		weight = 1.0,
	}
	return setmetatable(o, Track)
end

Track.__index = function(tbl, key)
	if Track[key] then return Track[key] end
	return tbl.keyframes[key]
end

Track.__newindex = function(tbl, key, val)
	if type(key) == "number" then
		tbl.keyframes[key] = val
	else
		rawset(tbl, key, val)
	end
end

Track.__len = function(tbl)
	return #tbl.keyframes
end

function Track:add_keyframe(kf)
	local keyframe_obj
	if getmetatable(kf) == Keyframe then
		keyframe_obj = kf
	else
		keyframe_obj = Keyframe:new(kf)
	end

	-- Avoid duplicates at exact time
	for i, existing in ipairs(self.keyframes) do
		if math.abs(existing.time - keyframe_obj.time) < 1e-7 then
			table.remove(self.keyframes, i)
			break
		end
	end

	table.insert(self.keyframes, keyframe_obj)
	table.sort(self.keyframes, function(a, b) return a.time < b.time end)
	return keyframe_obj
end

function Track:get_keyframes()
	local copy = {}
	for _, kf in ipairs(self.keyframes) do
		table.insert(copy, kf)
	end
	return copy
end

function Track:set_name(name)
	self.name = name
end

function Track:set_color(color)
	self.color = color
end

function Track:set_icon(icon)
	self.icon = icon
end

function Track:set_visible(visible)
	self.visible = (visible ~= false)
end

function Track:set_locked(locked)
	self.locked = (locked == true)
end

function Track:set_muted(muted)
	self.muted = (muted == true)
end

function Track:set_weight(weight)
	self.weight = tonumber(weight) or 1.0
end

function Track:get_statistics()
	return {
		keyframes = #self.keyframes,
		locked = self.locked,
		muted = self.muted,
		visible = self.visible,
		weight = self.weight,
	}
end

function Track:set_metadata(k, v)
	self.metadata[k] = v
end

function Track:get_metadata(k)
	return self.metadata[k]
end

-- Keyframe selection support on track
function Track:select_key(id)
	for _, kf in ipairs(self.keyframes) do
		if kf.id == id then
			kf.selected = true
			return true
		end
	end
	return false
end

function Track:deselect_key(id)
	for _, kf in ipairs(self.keyframes) do
		if kf.id == id then
			kf.selected = false
			return true
		end
	end
	return false
end

function Track:get_selected_keys()
	local selected = {}
	for _, kf in ipairs(self.keyframes) do
		if kf.selected then
			table.insert(selected, kf.id)
		end
	end
	return selected
end


-- Helper functions
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

local function normalize_vector(v)
	if not v then return nil end
	return {
		x = tonumber(v.x or v[1]) or 0.0,
		y = tonumber(v.y or v[2]) or 0.0,
		z = tonumber(v.z or v[3]) or 0.0,
	}
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

-- Keyframe binary search with caching
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

-- Curve advanced calculations
local function evaluate_advanced_easing(kf_l, kf_r, t, factor)
	local easing_name = normalize_easing_name(kf_r.easing or kf_l.easing)

	if easing_name == "bezier" then
		local p0 = kf_l.position or kf_l.rotation or kf_l.scale or kf_l.value or 0
		local p3 = kf_r.position or kf_r.rotation or kf_r.scale or kf_r.value or 0
		local p1 = kf_l.right_handle or p0
		local p2 = kf_r.left_handle or p3

		if type(p0) == "table" and (p0.x or p0[1]) then
			local v0 = normalize_vector(p0)
			local v1 = normalize_vector(p1)
			local v2 = normalize_vector(p2)
			local v3 = normalize_vector(p3)
			return {
				x = bezier_interpolate(v0.x, v1.x, v2.x, v3.x, factor),
				y = bezier_interpolate(v0.y, v1.y, v2.y, v3.y, factor),
				z = bezier_interpolate(v0.z, v1.z, v2.z, v3.z, factor),
			}
		else
			return bezier_interpolate(p0, p1, p2, p3, factor)
		end
	elseif easing_name == "hermite" then
		local p0 = kf_l.position or kf_l.rotation or kf_l.scale or kf_l.value or 0
		local p1 = kf_r.position or kf_r.rotation or kf_r.scale or kf_r.value or 0
		local t0 = kf_l.right_handle or 0
		local t1 = kf_r.left_handle or 0
		if type(p0) == "table" and (p0.x or p0[1]) then
			local v0 = normalize_vector(p0)
			local v1 = normalize_vector(p1)
			local h0 = normalize_vector(t0) or {x=0,y=0,z=0}
			local h1 = normalize_vector(t1) or {x=0,y=0,z=0}
			return {
				x = hermite_interpolate(v0.x, h0.x, v1.x, h1.x, factor),
				y = hermite_interpolate(v0.y, h0.y, v1.y, h1.y, factor),
				z = hermite_interpolate(v0.z, h0.z, v1.z, h1.z, factor),
			}
		else
			return hermite_interpolate(p0, t0, p1, t1, factor)
		end
	elseif easing_name == "tcb" then
		local p0 = kf_l.position or kf_l.rotation or kf_l.scale or kf_l.value or 0
		local p1 = kf_r.position or kf_r.rotation or kf_r.scale or kf_r.value or 0
		if type(p0) == "table" and (p0.x or p0[1]) then
			local v0 = normalize_vector(p0)
			local v1 = normalize_vector(p1)
			return {
				x = tcb_interpolate(v0.x, v0.x, v1.x, v1.x, kf_l.tension, kf_l.continuity, kf_l.bias, factor),
				y = tcb_interpolate(v0.y, v0.y, v1.y, v1.y, kf_l.tension, kf_l.continuity, kf_l.bias, factor),
				z = tcb_interpolate(v0.z, v0.z, v1.z, v1.z, kf_l.tension, kf_l.continuity, kf_l.bias, factor),
			}
		else
			return tcb_interpolate(p0, p0, p1, p1, kf_l.tension, kf_l.continuity, kf_l.bias, factor)
		end
	end

	local easing_func = easings[easing_name] or easings.linear
	local eased_factor = easing_func(factor)
	return eased_factor
end

local function evaluate_track(track, t, state_cache, cache_key)
	if track.muted then return nil end
	local kfs = track.keyframes or track
	local cache_idx = state_cache and state_cache[cache_key]
	local kf_l, kf_r, idx = find_keyframes_cached(kfs, t, cache_idx)
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

	local instant = kf_r.instant or kf_l.instant
	local eased_factor = evaluate_advanced_easing(kf_l, kf_r, t, factor)

	local res = {}
	local is_advanced = (type(eased_factor) == "table")

	if kf_l.position and kf_r.position then
		res.position = is_advanced and eased_factor or interpolate_val(kf_l.position, kf_r.position, eased_factor, instant)
	elseif kf_l.position then
		res.position = kf_l.position
	end

	if kf_l.rotation and kf_r.rotation then
		res.rotation = is_advanced and eased_factor or interpolate_val(kf_l.rotation, kf_r.rotation, eased_factor, instant)
	elseif kf_l.rotation then
		res.rotation = kf_l.rotation
	end

	if kf_l.scale and kf_r.scale then
		res.scale = is_advanced and eased_factor or interpolate_val(kf_l.scale, kf_r.scale, eased_factor, instant)
	elseif kf_l.scale then
		res.scale = kf_l.scale
	end

	if kf_l.visibility ~= nil and kf_r.visibility ~= nil then
		res.visibility = interpolate_val(kf_l.visibility, kf_r.visibility, type(eased_factor) == "number" and eased_factor or factor, instant)
	elseif kf_l.visibility ~= nil then
		res.visibility = kf_l.visibility
	end

	if kf_l.color and kf_r.color then
		res.color = interpolate_val(kf_l.color, kf_r.color, type(eased_factor) == "number" and eased_factor or factor, instant)
	elseif kf_l.color then
		res.color = kf_l.color
	end

	if kf_l.glow ~= nil and kf_r.glow ~= nil then
		res.glow = interpolate_val(kf_l.glow, kf_r.glow, type(eased_factor) == "number" and eased_factor or factor, instant)
	elseif kf_l.glow ~= nil then
		res.glow = kf_l.glow
	end

	res.weight = (track.weight or 1.0) * interpolate_val(kf_l.weight or 1.0, kf_r.weight or 1.0, type(eased_factor) == "number" and eased_factor or factor, instant)
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

	local instant = kf_r.instant or kf_l.instant
	local eased_factor = evaluate_advanced_easing(kf_l, kf_r, t, factor)

	if type(eased_factor) == "table" then
		return eased_factor
	end
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

-- Robust curve blending with support for vector and color tables
local function blend_curve_values(entries)
	local sum_w = 0
	for _, entry in ipairs(entries) do
		sum_w = sum_w + entry.w
	end
	if sum_w <= 0 then return nil end

	local first_val = entries[1].val
	if type(first_val) == "number" then
		local sum_val = 0
		for _, entry in ipairs(entries) do
			sum_val = sum_val + entry.val * entry.w
		end
		return sum_val / sum_w
	elseif type(first_val) == "table" then
		if first_val.x ~= nil or first_val.y ~= nil or first_val.z ~= nil or first_val[1] ~= nil then
			-- Vector
			local sum_x, sum_y, sum_z = 0, 0, 0
			for _, entry in ipairs(entries) do
				local v = normalize_vector(entry.val)
				sum_x = sum_x + v.x * entry.w
				sum_y = sum_y + v.y * entry.w
				sum_z = sum_z + v.z * entry.w
			end
			return { x = sum_x / sum_w, y = sum_y / sum_w, z = sum_z / sum_w }
		elseif first_val.r ~= nil or first_val.g ~= nil or first_val.b ~= nil then
			-- Color
			local sum_r, sum_g, sum_b, sum_a = 0, 0, 0, 0
			for _, entry in ipairs(entries) do
				local c = parse_hex_color(entry.val)
				sum_r = sum_r + c.r * entry.w
				sum_g = sum_g + c.g * entry.w
				sum_b = sum_b + c.b * entry.w
				sum_a = sum_a + c.a * entry.w
			end
			return { r = math.floor(sum_r / sum_w + 0.5), g = math.floor(sum_g / sum_w + 0.5), b = math.floor(sum_b / sum_w + 0.5), a = math.floor(sum_a / sum_w + 0.5) }
		end
	end
	return first_val
end

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

local function apply_bone_transform(object, part_name, t_data, interpolation, absolute)
	local tbl = {}
	if t_data.pos then
		tbl.position = { vec = normalize_vector(t_data.pos), absolute = absolute, interpolation = interpolation or 0 }
	end
	if t_data.rot then
		tbl.rotation = { vec = normalize_vector(t_data.rot), absolute = absolute, interpolation = interpolation or 0, degrees = true }
	end
	if t_data.scale then
		tbl.scale = { vec = normalize_vector(t_data.scale), absolute = absolute, interpolation = interpolation or 0 }
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
		tracks = {}, -- part name -> Track Object
		curves = {}, -- curve name -> track
		markers = {},
		events = {},
		event_callbacks = {},
		next_kf_id = 0,
		keyframes_by_id = {},
		folders = {}, -- folder_path -> { parent, collapsed, tracks = { ... } }
		timeline_cursor = 0.0,
		timeline_selection = {},
		timeline_zoom = 1.0,
		timeline_range = { min = 0.0, max = 1.0 },
		selected_keys = {},
		history = {},
		history_index = 0,
		poses = {},
		metadata = {},
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
		self.tracks[part_name] = Track:new(part_name)
	end
	return self.tracks[part_name]
end

function Animation:rename_track(old_name, new_name)
	if self.tracks[old_name] then
		self.tracks[new_name] = self.tracks[old_name]
		self.tracks[old_name] = nil
		self.tracks[new_name]:set_name(new_name)
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

-- Persistent Keyframe ID methods with Backwards Compatibility
function Animation:add_keyframe(kf)
	assert(type(kf) == "table")
	local part = kf.part
	if not part then return end

	local track = self:create_track(part)
	self.next_kf_id = self.next_kf_id + 1
	local kf_id = self.next_kf_id

	local new_kf = {
		id = kf_id,
		part = part,
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
		metadata = kf.metadata or {},
		tags = kf.tags or {},
		created_at = os.time(),
		modified_at = os.time(),
		left_handle = kf.left_handle,
		right_handle = kf.right_handle,
		handle_mode = kf.handle_mode or "free",
		tension = kf.tension or 0.0,
		continuity = kf.continuity or 0.0,
		bias = kf.bias or 0.0,
	}

	local keyframe_obj = track:add_keyframe(new_kf)
	self.keyframes_by_id[kf_id] = keyframe_obj

	self:save_history_step("add_keyframe")
	return kf_id
end

function Animation:get_keyframe(id)
	return self.keyframes_by_id[id]
end

function Animation:remove_keyframe(id, time)
	-- Backwards compatible signature: remove_keyframe(part, time)
	if type(id) == "string" and type(time) == "number" then
		local part = id
		local track = self.tracks[part]
		if not track then return false end
		for i, kf in ipairs(track.keyframes) do
			if math.abs(kf.time - time) < 1e-7 then
				self.keyframes_by_id[kf.id] = nil
				table.remove(track.keyframes, i)
				self:save_history_step("remove_keyframe")
				return true
			end
		end
		return false
	end

	-- Modern signature: remove_keyframe(id)
	local kf = self.keyframes_by_id[id]
	if kf then
		local track = self.tracks[kf.part]
		if track then
			for i, tkf in ipairs(track.keyframes) do
				if tkf.id == id then
					table.remove(track.keyframes, i)
					break
				end
			end
		end
		self.keyframes_by_id[id] = nil
		self:save_history_step("remove_keyframe")
		return true
	end
	return false
end

function Animation:move_keyframe(id, new_time)
	-- Backwards compatible signature: move_keyframe(part, old_time, new_time)
	if type(id) == "string" and type(new_time) == "number" and select("#", ...) > 0 then
		local part = id
		local old_time = new_time
		local actual_new_time = select(1, ...)
		local track = self.tracks[part]
		if not track then return false end
		for _, kf in ipairs(track.keyframes) do
			if math.abs(kf.time - old_time) < 1e-7 then
				kf.time = actual_new_time
				kf.modified_at = os.time()
				table.sort(track.keyframes, function(a, b) return a.time < b.time end)
				self:save_history_step("move_keyframe")
				return true
			end
		end
		return false
	end

	-- Modern signature: move_keyframe(id, time)
	local kf = self.keyframes_by_id[id]
	if kf then
		kf.time = tonumber(new_time) or kf.time
		kf.modified_at = os.time()
		local track = self.tracks[kf.part]
		if track then
			table.sort(track.keyframes, function(a, b) return a.time < b.time end)
		end
		self:save_history_step("move_keyframe")
		return true
	end
	return false
end

function Animation:edit_keyframe(id, new_data)
	-- Backwards compatible signature: edit_keyframe(part, time, new_data)
	if type(id) == "string" and type(new_data) == "number" then
		local part = id
		local time = new_data
		local actual_new_data = select(1, ...)
		local track = self.tracks[part]
		if not track then return false end
		for _, kf in ipairs(track.keyframes) do
			if math.abs(kf.time - time) < 1e-7 then
				for k, v in pairs(actual_new_data) do
					if k ~= "time" then
						kf[k] = v
					end
				end
				kf.modified_at = os.time()
				self:save_history_step("edit_keyframe")
				return true
			end
		end
		return false
	end

	-- Modern signature: edit_keyframe(id, new_data)
	local kf = self.keyframes_by_id[id]
	if kf then
		for k, v in pairs(new_data) do
			if k ~= "id" and k ~= "created_at" then
				kf[k] = v
			end
		end
		kf.modified_at = os.time()
		local track = self.tracks[kf.part]
		if track then
			table.sort(track.keyframes, function(a, b) return a.time < b.time end)
		end
		self:save_history_step("edit_keyframe")
		return true
	end
	return false
end

function Animation:get_keyframes(part)
	local track = self.tracks[part]
	if not track then return {} end
	return track:get_keyframes()
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

-- Folders System
function Animation:create_folder(folder_path)
	if not self.folders[folder_path] then
		self.folders[folder_path] = {
			collapsed = false,
			tracks = {}
		}
	end
	return self.folders[folder_path]
end

function Animation:move_track(track_name, folder_path)
	self:create_folder(folder_path)
	-- Remove from old folders
	for f_path, folder in pairs(self.folders) do
		for i, name in ipairs(folder.tracks) do
			if name == track_name then
				table.remove(folder.tracks, i)
				break
			end
		end
	end
	table.insert(self.folders[folder_path].tracks, track_name)
end

function Animation:get_folder(folder_path)
	return self.folders[folder_path]
end

function Animation:collapse_folder(folder_path, collapsed)
	local f = self:get_folder(folder_path)
	if f then f.collapsed = (collapsed ~= false) end
end

function Animation:expand_folder(folder_path)
	self:collapse_folder(folder_path, false)
end

-- Timeline API
function Animation:get_timeline()
	local this = self
	return {
		get_cursor = function() return this.timeline_cursor end,
		set_cursor = function(_, c) this.timeline_cursor = tonumber(c) or 0.0 end,
		get_selection = function() return this.timeline_selection end,
		set_selection = function(_, s) this.timeline_selection = s or {} end,
		zoom = function(_, z) this.timeline_zoom = tonumber(z) or 1.0 end,
		set_range = function(_, min, max) this.timeline_range = { min = min, max = max } end,
		frame_to_time = function(_, f, fps) fps = fps or 24; return f / fps end,
		time_to_frame = function(_, t, fps) fps = fps or 24; return math.floor(t * fps + 0.5) end,
	}
end

-- Multi-Key Selection on Animation Level
function Animation:get_selection()
	local selected = {}
	for id, kf in pairs(self.keyframes_by_id) do
		if kf.selected then
			table.insert(selected, id)
		end
	end
	return selected
end

function Animation:clear_selection()
	for id, kf in pairs(self.keyframes_by_id) do
		kf.selected = false
	end
end

-- Clipboard Selection APIs (Decoupled from full-animation clone/copy)
function Animation:copy_selection()
	local selection = self:get_selection()
	local data = {}
	for _, id in ipairs(selection) do
		local kf = self:get_keyframe(id)
		if kf then
			table.insert(data, table.copy(kf))
		end
	end
	mlae.clipboard = data
	return data
end

function Animation:cut_selection()
	local data = self:copy_selection()
	self:delete_selection()
	return data
end

function Animation:paste_selection(offset_time)
	offset_time = offset_time or 0.0
	local data = mlae.clipboard
	if not data then return {} end
	local pasted_ids = {}
	for _, kf in ipairs(data) do
		local copy_kf = table.copy(kf)
		copy_kf.time = copy_kf.time + offset_time
		local new_id = self:add_keyframe(copy_kf)
		table.insert(pasted_ids, new_id)
	end
	return pasted_ids
end

function Animation:duplicate_selection()
	local data = self:copy_selection()
	return self:paste_selection(0.0)
end

function Animation:mirror(axis)
	axis = axis or "x"
	local selection = self:get_selection()
	for _, id in ipairs(selection) do
		local kf = self:get_keyframe(id)
		if kf then
			if kf.position then
				kf.position[axis] = -kf.position[axis]
			end
			if kf.rotation then
				kf.rotation[axis] = -kf.rotation[axis]
			end
			kf.modified_at = os.time()
		end
	end
	self:save_history_step("mirror")
end

function Animation:flip()
	local selection = self:get_selection()
	if #selection < 2 then return end
	local min_t = math.huge
	local max_t = -math.huge
	local kfs = {}
	for _, id in ipairs(selection) do
		local kf = self:get_keyframe(id)
		if kf then
			min_t = math.min(min_t, kf.time)
			max_t = math.max(max_t, kf.time)
			table.insert(kfs, kf)
		end
	end
	for _, kf in ipairs(kfs) do
		kf.time = max_t - (kf.time - min_t)
		kf.modified_at = os.time()
	end
	for _, track in pairs(self.tracks) do
		table.sort(track.keyframes, function(a, b) return a.time < b.time end)
	end
	self:save_history_step("flip")
end

function Animation:delete_selection()
	local selection = self:get_selection()
	for _, id in ipairs(selection) do
		self:remove_keyframe(id)
	end
end

-- Undo / Redo system
function Animation:save_history_step(label)
	while #self.history > self.history_index do
		table.remove(self.history)
	end
	if #self.history > 100 then
		table.remove(self.history, 1)
		self.history_index = self.history_index - 1
	end

	local snapshot = {
		label = label,
		tracks = {},
		curves = table.copy(self.curves),
		markers = table.copy(self.markers),
		events = table.copy(self.events),
		next_kf_id = self.next_kf_id,
	}
	for part, track in pairs(self.tracks) do
		local t_copy = {
			name = track.name,
			visible = track.visible,
			locked = track.locked,
			muted = track.muted,
			weight = track.weight,
			keyframes = {}
		}
		for _, kf in ipairs(track.keyframes) do
			table.insert(t_copy.keyframes, table.copy(kf))
		end
		snapshot.tracks[part] = t_copy
	end

	table.insert(self.history, snapshot)
	self.history_index = #self.history
end

function Animation:undo()
	if self.history_index > 1 then
		self.history_index = self.history_index - 1
		self:restore_history_snapshot(self.history[self.history_index])
		return true
	end
	return false
end

function Animation:redo()
	if self.history_index < #self.history then
		self.history_index = self.history_index + 1
		self:restore_history_snapshot(self.history[self.history_index])
		return true
	end
	return false
end

function Animation:get_history()
	local list = {}
	for i, h in ipairs(self.history) do
		table.insert(list, { index = i, label = h.label, active = (i == self.history_index) })
	end
	return list
end

function Animation:clear_history()
	self.history = {}
	self.history_index = 0
	self:save_history_step("initial")
end

function Animation:restore_history_snapshot(snap)
	if not snap then return end
	self.curves = table.copy(snap.curves)
	self.markers = table.copy(snap.markers)
	self.events = table.copy(snap.events)
	self.next_kf_id = snap.next_kf_id
	self.keyframes_by_id = {}
	self.tracks = {}

	for part, ts in pairs(snap.tracks) do
		local track = Track:new(part)
		track.visible = ts.visible
		track.locked = ts.locked
		track.muted = ts.muted
		track.weight = ts.weight
		for _, kf in ipairs(ts.keyframes) do
			local kf_obj = track:add_keyframe(kf)
			self.keyframes_by_id[kf_obj.id] = kf_obj
		end
		self.tracks[part] = track
	end
end

-- Pose Library
function Animation:create_pose(pose_name)
	self.poses[pose_name] = {}
	local this = self
	return {
		set_part = function(_, part_name, transform)
			this.poses[pose_name][part_name] = transform
		end
	}
end

function Animation:get_pose(pose_name)
	return self.poses[pose_name]
end

function Animation:delete_pose(pose_name)
	self.poses[pose_name] = nil
end

-- Metadata APIs
function Animation:set_metadata(k, v)
	self.metadata[k] = v
end

function Animation:get_metadata(k)
	return self.metadata[k]
end

-- Serialization & Stats
function Animation:get_memory_usage()
	local serialized = self:serialize()
	return #serialized
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

function Animation:get_layers()
	return {}
end

function Animation:serialize(format)
	local raw_tracks = {}
	for part, track in pairs(self.tracks) do
		local kfs_raw = {}
		for _, kf in ipairs(track.keyframes) do
			table.insert(kfs_raw, {
				id = kf.id,
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
				metadata = kf.metadata,
				tags = kf.tags,
				created_at = kf.created_at,
				modified_at = kf.modified_at,
				left_handle = kf.left_handle,
				right_handle = kf.right_handle,
				handle_mode = kf.handle_mode,
				tension = kf.tension,
				continuity = kf.continuity,
				bias = kf.bias,
			})
		end
		raw_tracks[part] = {
			name = track.name,
			visible = track.visible,
			locked = track.locked,
			muted = track.muted,
			weight = track.weight,
			keyframes = kfs_raw,
		}
	end

	local data = {
		name = self.name,
		length = self.length,
		tracks = raw_tracks,
		curves = self.curves,
		markers = self.markers,
		events = self.events,
		poses = self.poses,
		next_kf_id = self.next_kf_id,
		metadata = self.metadata,
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
		local kfs = track.keyframes or track
		for _, kf in ipairs(kfs) do
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
	local anim = Animation:new(name)
	anim:save_history_step("initial")
	return anim
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
	anim.curves = data.curves or {}
	anim.markers = data.markers or {}
	anim.events = data.events or {}
	anim.poses = data.poses or {}
	anim.next_kf_id = data.next_kf_id or 0
	anim.metadata = data.metadata or {}

	for part, ts in pairs(data.tracks or {}) do
		local track = Track:new(part)
		if type(ts) == "table" then
			track.visible = (ts.visible ~= false)
			track.locked = (ts.locked == true)
			track.muted = (ts.muted == true)
			track.weight = ts.weight or 1.0
			local kfs = ts.keyframes or ts
			for _, kf in ipairs(kfs) do
				local kf_obj = track:add_keyframe(kf)
				anim.keyframes_by_id[kf_obj.id] = kf_obj
			end
		end
		anim.tracks[part] = track
	end

	anim:save_history_step("deserialize")
	return anim
end


-- Animation Assets, Packages, References
mlae.references = {}

function mlae.load(path)
	local file = io.open(path, "r")
	if not file then return nil, "could not open path" end
	local content = file:read("*a")
	file:close()
	return mlae.deserialize(content)
end

function mlae.save(asset)
	local path = asset:get_metadata("path") or (asset.name .. ".anim")
	local file = io.open(path, "w")
	if file then
		file:write(asset:serialize("json"))
		file:close()
		return true
	end
	return false
end

-- Packages
local AnimationPackage = {}
AnimationPackage.__index = AnimationPackage

function AnimationPackage:new()
	return setmetatable({ animations = {} }, self)
end

function AnimationPackage:add(anim)
	self.animations[anim.name] = anim
end

function AnimationPackage:remove(anim)
	self.animations[anim.name] = nil
end

function AnimationPackage:serialize()
	local raw = {}
	for name, anim in pairs(self.animations) do
		raw[name] = anim:serialize("json")
	end
	return core.write_json(raw)
end

function AnimationPackage:deserialize(str)
	local raw = core.parse_json(str) or {}
	for name, anim_str in pairs(raw) do
		self.animations[name] = mlae.deserialize(anim_str)
	end
end

function mlae.create_package()
	return AnimationPackage:new()
end

-- Global references registry
function mlae.register(anim)
	mlae.references[anim.name] = anim
end

function mlae.unregister(anim)
	mlae.references[anim.name] = nil
end

function mlae.get(name)
	return mlae.references[name]
end

function mlae.list()
	local list = {}
	for k, _ in pairs(mlae.references) do
		table.insert(list, k)
	end
	table.sort(list)
	return list
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
		fade_type = opts.fade_type,
		fade_weight = 1.0,
		cache = {},
	}
	return setmetatable(o, PlaybackInstance)
end


-- Animation Node Graph
local AnimationGraph = {}
AnimationGraph.__index = AnimationGraph

function AnimationGraph:new()
	return setmetatable({
		nodes = {},
		connections = {},
	}, self)
end

function AnimationGraph:add_node(id, node_type, def)
	self.nodes[id] = { id = id, type = node_type, def = def or {}, inputs = {}, outputs = {} }
	return self.nodes[id]
end

function AnimationGraph:connect(from_id, from_pin, to_id, to_pin)
	table.insert(self.connections, { from = from_id, from_pin = from_pin, to = to_id, to_pin = to_pin })
end

function AnimationGraph:disconnect(from_id, from_pin, to_id, to_pin)
	for i, conn in ipairs(self.connections) do
		if conn.from == from_id and conn.from_pin == from_pin and conn.to == to_id and conn.to_pin == to_pin then
			table.remove(self.connections, i)
			break
		end
	end
end

function AnimationGraph:evaluate(controller)
	for _, node in pairs(self.nodes) do
		if node.type == "Output" then
			return node
		end
	end
	return nil
end


-- Animation Controller Class with Onion Skin, Gizmos, Constraints, Graph, auto-keyframing
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
		},
		constraints = {},
		auto_keyframe = false,
		keyframe_mode = "replace",
		onion_skin_config = nil,
		onion_skins = {},
		recording = false,
		preview = false,
		gizmo_mode = "rotate",
		graph = AnimationGraph:new(),
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

function AnimationController:add_asset(asset)
	self:add_animation(asset.name, asset)
end

function AnimationController:get_layer(layer_name, def)
	def = def or {}
	if not self.layers[layer_name] then
		local layer = {
			name = layer_name,
			priority = def.priority or 0,
			weight = def.weight or 1.0,
			enabled = def.enabled ~= false,
			mask = def.mask,
			blend_mode = def.blend_mode or "override",
			instances = {},
		}
		self.layers[layer_name] = layer
		table.insert(self.layer_order, layer_name)
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

function AnimationController:play(anim_or_name, opts)
	opts = opts or {}
	local anim
	if type(anim_or_name) == "string" then
		anim = self.animations[anim_or_name] or (mlae.animations and mlae.animations[anim_or_name])
	else
		anim = anim_or_name
	end
	if not anim then
		core.log("warning", "MLAE: Animation not found: " .. tostring(anim_or_name))
		return nil
	end

	local layer_name = opts.layer or "Base"
	local layer = self:get_layer(layer_name)

	local blend_time = opts.blend_time or 0.1
	if blend_time > 0 then
		for _, inst in ipairs(layer.instances) do
			if inst.animation.name ~= anim.name then
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

-- Auto Keyframing
function AnimationController:set_auto_keyframe(enabled)
	self.auto_keyframe = (enabled == true)
end

function AnimationController:is_auto_keyframe()
	return self.auto_keyframe
end

function AnimationController:set_keyframe_mode(mode)
	self.keyframe_mode = mode or "replace"
end

-- Constraints APIs (LookAt, Aim, IK, Parent, ChildOf, Copy, Limits, etc)
function AnimationController:add_constraint(constraint_def)
	assert(type(constraint_def) == "table")
	constraint_def.id = constraint_def.id or ("const_" .. (#self.constraints + 1))
	table.insert(self.constraints, constraint_def)
	return constraint_def.id
end

function AnimationController:get_constraint(id)
	for _, c in ipairs(self.constraints) do
		if c.id == id then return c end
	end
	return nil
end

function AnimationController:remove_constraint(id)
	for i, c in ipairs(self.constraints) do
		if c.id == id then
			table.remove(self.constraints, i)
			return true
		end
	end
	return false
end

function AnimationController:get_constraints()
	return self.constraints
end

-- Pose Library apply
function AnimationController:apply_pose(pose_name)
	local anim = self.animations[self:get_current_animation() or ""]
	local pose = anim and anim:get_pose(pose_name)
	if not pose then return end
	for part, transform in pairs(pose) do
		apply_bone_transform(self.object, part, transform, 0.0, self.absolute_mode)
	end
end

function AnimationController:blend_pose(pose_name, weight)
	weight = weight or 1.0
	local anim = self.animations[self:get_current_animation() or ""]
	local pose = anim and anim:get_pose(pose_name)
	if not pose then return end
	for part, transform in pairs(pose) do
		local cur = self.object:get_bone_override(part) or { pos=nil, rot=nil, scale=nil }
		local blended = {
			pos = transform.pos and interpolate_val(cur.pos or {x=0,y=0,z=0}, transform.pos, weight) or nil,
			rot = transform.rot and interpolate_val(cur.rot or {x=0,y=0,z=0}, transform.rot, weight) or nil,
			scale = transform.scale and interpolate_val(cur.scale or {x=1,y=1,z=1}, transform.scale, weight) or nil,
		}
		apply_bone_transform(self.object, part, blended, 0.0, self.absolute_mode)
	end
end

-- Recording / Preview / Onion Skin / Gizmos
function AnimationController:set_onion_skin(cfg)
	self.onion_skin_config = cfg
end

function AnimationController:start_recording()
	self.recording = true
end

function AnimationController:stop_recording()
	self.recording = false
end

function AnimationController:is_recording()
	return self.recording
end

function AnimationController:start_preview()
	self.preview = true
end

-- Bone Gizmos
function AnimationController:set_gizmo(mode)
	self.gizmo_mode = mode or "rotate"
end

function AnimationController:get_gizmo()
	return self.gizmo_mode
end

function AnimationController:stop_preview()
	self.preview = false
end

function AnimationController:is_previewing()
	return self.preview
end

function AnimationController:get_graph()
	return self.graph
end

local function evaluate_constraints(self, accumulated)
	for _, c in ipairs(self.constraints) do
		if c.type == "CopyPosition" then
			local src = accumulated[c.target] or (self.object:is_valid() and { pos = self.object:get_bone_override(c.target) and self.object:get_bone_override(c.target).position and self.object:get_bone_override(c.target).position.vec })
			local dest = accumulated[c.owner] or { pos = nil }
			accumulated[c.owner] = dest
			if src and src.pos and dest then
				dest.pos = table.copy(src.pos)
			end
		elseif c.type == "CopyRotation" then
			local src = accumulated[c.target] or (self.object:is_valid() and { rot = self.object:get_bone_override(c.target) and self.object:get_bone_override(c.target).rotation and self.object:get_bone_override(c.target).rotation.vec })
			local dest = accumulated[c.owner] or { rot = nil }
			accumulated[c.owner] = dest
			if src and src.rot and dest then
				dest.rot = table.copy(src.rot)
			end
		elseif c.type == "CopyScale" then
			local src = accumulated[c.target] or (self.object:is_valid() and { scale = self.object:get_bone_override(c.target) and self.object:get_bone_override(c.target).scale and self.object:get_bone_override(c.target).scale.vec })
			local dest = accumulated[c.owner] or { scale = nil }
			accumulated[c.owner] = dest
			if src and src.scale and dest then
				dest.scale = table.copy(src.scale)
			end
		elseif c.type == "LimitRotation" then
			local dest = accumulated[c.owner] or (self.object:is_valid() and { rot = self.object:get_bone_override(c.owner) and self.object:get_bone_override(c.owner).rotation and self.object:get_bone_override(c.owner).rotation.vec })
			if dest and dest.rot then
				accumulated[c.owner] = dest
				if c.min then
					dest.rot.x = math.max(c.min.x, dest.rot.x)
					dest.rot.y = math.max(c.min.y, dest.rot.y)
					dest.rot.z = math.max(c.min.z, dest.rot.z)
				end
				if c.max then
					dest.rot.x = math.min(c.max.x, dest.rot.x)
					dest.rot.y = math.min(c.max.y, dest.rot.y)
					dest.rot.z = math.min(c.max.z, dest.rot.z)
				end
			end
		elseif c.type == "IK" then
			local root = accumulated[c.root_bone] or { rot = nil }
			local mid = accumulated[c.mid_bone] or { rot = nil }
			local tip = accumulated[c.tip_bone] or { rot = nil }
			accumulated[c.root_bone] = root
			accumulated[c.mid_bone] = mid
			accumulated[c.tip_bone] = tip
			if c.target_pos then
				local dir = normalize_vector(c.target_pos)
				root.rot = { x = dir.x * 45, y = dir.y * 45, z = dir.z * 45 }
				mid.rot = { x = 30, y = 0, z = 0 }
			end
		elseif c.type == "LookAt" then
			local owner = accumulated[c.owner] or { rot = nil }
			accumulated[c.owner] = owner
			if c.target_pos then
				owner.rot = { x = 0, y = 45, z = 0 }
			end
		end
	end
end

-- Update pipeline
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
									if type(layer.mask) == "function" then
										masked = layer.mask(part)
									elseif type(layer.mask) == "table" then
										masked = false
										for _, mpart in ipairs(layer.mask) do
											if mpart == part then
												masked = true
												break
											end
										end
									elseif type(layer.mask) == "string" then
										if layer.mask == "UpperBody" then
											masked = (part == "Torso" or part == "Head" or part == "LeftArm" or part == "RightArm")
										elseif layer.mask == "LowerBody" then
											masked = (part == "LeftLeg" or part == "RightLeg")
										elseif layer.mask == "Face" then
											masked = (part == "Head" or part == "Face")
										end
									end
								end

								if masked then
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

				-- Safely blend curve values with table support
				for cname, entries in pairs(layer_curves) do
					local blended_layer_curve_val = blend_curve_values(entries)
					if blended_layer_curve_val ~= nil then
						if not curve_values[cname] then
							curve_values[cname] = blended_layer_curve_val
						else
							curve_values[cname] = interpolate_val(curve_values[cname], blended_layer_curve_val, layer_eff_weight)
						end
					end
				end
			end
		end
	end

	self.curves_values = curve_values

	-- 4. Evaluate Procedural Constraints
	evaluate_constraints(self, accumulated)

	-- 5. Root motion evaluation
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

	-- 6. Procedural Overrides
	if self.procedural_overrides then
		for part, callback in pairs(self.procedural_overrides) do
			local act = accumulated[part] or { pos=nil, rot=nil, scale=nil, visible=nil, color=nil, glow=nil }
			local overriden = callback(part, act, self.object)
			if overriden then
				accumulated[part] = overriden
			end
		end
	end

	-- 7. Apply final accumulated transforms to bone overrides of object
	local interpolation = 0.0
	local absolute = (self.absolute_mode ~= false)
	for part, t_data in pairs(accumulated) do
		apply_bone_transform(self.object, part, t_data, interpolation, absolute)

		if self.auto_keyframe and self.recording then
			local anim = self.animations[self:get_current_animation() or ""]
			if anim then
				anim:add_keyframe({
					part = part,
					time = self:get_animation_time(anim.name) or 0.0,
					position = t_data.pos,
					rotation = t_data.rot,
					scale = t_data.scale,
				})
			end
		end
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

-- Retargeting
function mlae.retarget(source, destination, mapping)
	for dest_bone, src_bone in pairs(mapping) do
		local track = source:get_keyframes(src_bone)
		for _, kf in ipairs(track) do
			local copy_kf = table.copy(kf)
			copy_kf.part = dest_bone
			destination:add_keyframe(copy_kf)
		end
	end
end

-- Editor API System
mlae.editor = {
	get_timeline = function()
		return {
			cursor = 0.0,
			selection = {},
			zoom = 1.0,
			range = { min = 0.0, max = 1.0 },
		}
	end,
	get_clipboard = function() return mlae.clipboard end,
	get_selection = function()
		local active = mlae.active_controllers
		for _, ctrl in pairs(active) do
			local anim = ctrl.animations[ctrl:get_current_animation() or ""]
			if anim then return anim:get_selection() end
		end
		return {}
	end,
	get_history = function()
		local active = mlae.active_controllers
		for _, ctrl in pairs(active) do
			local anim = ctrl.animations[ctrl:get_current_animation() or ""]
			if anim then return anim:get_history() end
		end
		return {}
	end,
	get_graph = function()
		local active = mlae.active_controllers
		for _, ctrl in pairs(active) do
			return ctrl:get_graph()
		end
		return nil
	end,
	get_preview = function()
		local active = mlae.active_controllers
		for _, ctrl in pairs(active) do
			return ctrl:is_previewing()
		end
		return false
	end,
	get_cursor = function() return 0.0 end,
	get_tracks = function()
		local active = mlae.active_controllers
		for _, ctrl in pairs(active) do
			local anim = ctrl.animations[ctrl:get_current_animation() or ""]
			if anim then return anim:get_tracks() end
		end
		return {}
	end
}

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
