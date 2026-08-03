-- Luanti Next-Generation Formspec Architecture (Modern UI Framework)
-- Copyright (C) 2010-2026 Perttu Ahola and contributors
-- SPDX-License-Identifier: LGPL-2.1-or-later

modern_ui = {}

-- Safe fallback of setfenv for Lua 5.2/5.3/5.4 compatibility
local setfenv = _G.setfenv or function(fn, env)
	local i = 1
	while true do
		local name = debug.getupvalue(fn, i)
		if name == "_ENV" then
			debug.setupvalue(fn, i, env)
			break
		elseif not name then
			break
		end
		i = i + 1
	end
	return fn
end

-- Defensive wrappers for core functions to prevent nil-errors
local safe_formspec_escape = function(text)
	if type(core) == "table" and type(core.formspec_escape) == "function" then
		return core.formspec_escape(text)
	end
	return tostring(text):gsub("\\", "\\\\"):gsub("%[", "\\x5b"):gsub("%]", "\\x5d"):gsub(";", "\\x3b"):gsub(",", "\\x2c")
end

local safe_show_formspec = function(playername, formname, formspec_str)
	if type(core) == "table" and type(core.show_formspec) == "function" then
		return core.show_formspec(playername, formname, formspec_str)
	end
	return false
end

-- =============================================================================
-- RE-USABLE STYLES AND THEMES
-- =============================================================================

modern_ui.styles = {}
modern_ui.themes = {}
modern_ui.current_theme = "dark"

function modern_ui.register_style(name, def)
	modern_ui.styles[name] = def
end

function modern_ui.register_theme(name, def)
	modern_ui.themes[name] = def
end

function modern_ui.set_theme(name)
	if modern_ui.themes[name] then
		modern_ui.current_theme = name
		return true
	end
	return false
end

-- Default Dark Theme
modern_ui.register_theme("dark", {
	window = { background = "#1E1E2E", color = "#C3BAC6", font_size = 14 },
	panel = { background = "#252538", border_color = "#45475A", border_width = 1, radius = 8 },
	label = { color = "#CDD6F4", font_size = 14 },
	button = {
		background = "#89B4FA",
		color = "#11111B",
		radius = 6,
		hover = { background = "#B4BEFE" },
		focused = { border_color = "#F5E0DC", border_width = 2 },
		disabled = { background = "#585B70", color = "#7F849C" }
	},
	checkbox = { color = "#CDD6F4" },
	toggle = { background = "#313244", active_background = "#A6E3A1" },
	slider = { track_color = "#313244", handle_color = "#89B4FA" },
	switch = { background = "#313244", active_background = "#A6E3A1" }
})

-- Default Light Theme
modern_ui.register_theme("light", {
	window = { background = "#EFF1F5", color = "#4C4F69", font_size = 14 },
	panel = { background = "#E6E9EF", border_color = "#CCD0DA", border_width = 1, radius = 8 },
	label = { color = "#4C4F69", font_size = 14 },
	button = {
		background = "#1E66F5",
		color = "#FFFFFF",
		radius = 6,
		hover = { background = "#7287FD" },
		focused = { border_color = "#DC8A78", border_width = 2 },
		disabled = { background = "#BCC0CC", color = "#8C8FA1" }
	},
	checkbox = { color = "#4C4F69" },
	toggle = { background = "#CCD0DA", active_background = "#40A02B" },
	slider = { track_color = "#CCD0DA", handle_color = "#1E66F5" },
	switch = { background = "#CCD0DA", active_background = "#40A02B" }
})

-- Game / High Contrast Theme
modern_ui.register_theme("game", {
	window = { background = "#000000", color = "#FFFFFF", font_size = 16 },
	panel = { background = "#111111", border_color = "#FFFFFF", border_width = 2, radius = 0 },
	label = { color = "#FFFFFF", font_size = 16 },
	button = {
		background = "#FFFF00",
		color = "#000000",
		radius = 0,
		hover = { background = "#FFA500" },
		focused = { border_color = "#FFFFFF", border_width = 3 },
		disabled = { background = "#555555", color = "#AAAAAA" }
	},
	checkbox = { color = "#FFFFFF" },
	toggle = { background = "#222222", active_background = "#FFFF00" },
	slider = { track_color = "#222222", handle_color = "#FFFF00" },
	switch = { background = "#222222", active_background = "#FFFF00" }
})


-- =============================================================================
-- DATA BINDING & REACTIVITY
-- =============================================================================

local Reactive = {}
Reactive.__index = Reactive

function modern_ui.reactive(initial_state)
	local state = {
		_values = initial_state or {},
		_listeners = {},
	}
	setmetatable(state, {
		__index = function(t, k)
			return t._values[k]
		end,
		__newindex = function(t, k, v)
			local old = t._values[k]
			if old ~= v then
				t._values[k] = v
				t:notify(k, v, old)
			end
		end
	})
	return setmetatable(state, Reactive)
end

function Reactive:bind(key, transform)
	return {
		is_binding = true,
		state = self,
		key = key,
		transform = transform,
	}
end

function Reactive:subscribe(key, callback)
	if not self._listeners[key] then
		self._listeners[key] = {}
	end
	table.insert(self._listeners[key], callback)
	local index = #self._listeners[key]
	return function()
		self._listeners[key][index] = nil
	end
end

function Reactive:notify(key, value, old_value)
	if self._listeners[key] then
		for _, cb in ipairs(self._listeners[key]) do
			if cb then
				pcall(cb, value, old_value)
			end
		end
	end
end


-- =============================================================================
-- EXTENSIBLE ANIMATION & EASING ENGINE
-- =============================================================================

modern_ui.active_animations = {}

local easings = {
	linear = function(t) return t end,
	easeIn = function(t) return t * t end,
	easeOut = function(t) return t * (2 - t) end,
	easeInOut = function(t)
		if t < 0.5 then return 2 * t * t else return -1 + (4 - 2 * t) * t end
	end,
	bounce = function(t)
		if t < 1 / 2.75 then
			return 7.5625 * t * t
		elseif t < 2 / 2.75 then
			t = t - 1.5 / 2.75
			return 7.5625 * t * t + 0.75
		elseif t < 2.5 / 2.75 then
			t = t - 2.25 / 2.75
			return 7.5625 * t * t + 0.9375
		else
			t = t - 2.625 / 2.75
			return 7.5625 * t * t + 0.984375
		end
	end,
	elastic = function(t)
		if t == 0 or t == 1 then return t end
		local p = 0.3
		return math.pow(2, -10 * t) * math.sin((t - p / 4) * (2 * math.pi) / p) + 1
	end,
	spring = function(t)
		return 1 - math.exp(-6 * t) * math.cos(12 * t)
	end
}

local function interpolate_num(start, dest, t)
	return start + (dest - start) * t
end

local function parse_color(hex)
	hex = tostring(hex):gsub("#", "")
	if #hex == 3 then
		local r = (tonumber(hex:sub(1,1), 16) or 15) * 17
		local g = (tonumber(hex:sub(2,2), 16) or 15) * 17
		local b = (tonumber(hex:sub(3,3), 16) or 15) * 17
		return { r = r, g = g, b = b }
	elseif #hex == 6 then
		local r = tonumber(hex:sub(1,2), 16)
		local g = tonumber(hex:sub(3,4), 16)
		local b = tonumber(hex:sub(5,6), 16)
		return { r = r or 255, g = g or 255, b = b or 255 }
	end
	return { r = 255, g = 255, b = 255 }
end

local function format_color(rgb)
	return string.format("#%02X%02X%02X", math.max(0, math.min(255, rgb.r)), math.max(0, math.min(255, rgb.g)), math.max(0, math.min(255, rgb.b)))
end

local function interpolate_color(start_hex, dest_hex, t)
	local c1 = parse_color(start_hex)
	local c2 = parse_color(dest_hex)
	return format_color({
		r = interpolate_num(c1.r, c2.r, t),
		g = interpolate_num(c1.g, c2.g, t),
		b = interpolate_num(c1.b, c2.b, t)
	})
end

if core.register_globalstep then
	core.register_globalstep(function(dtime)
		local dirty_forms = {}
		modern_ui.suppress_immediate_render = true

		for widget, anims in pairs(modern_ui.active_animations) do
			local widget_dirty = false
			for prop, anim in pairs(anims) do
				anim.elapsed = anim.elapsed + dtime
				local raw_t = math.min(1, anim.elapsed / anim.duration)
				local ease_fn = easings[anim.easing] or easings.linear
				local t = ease_fn(raw_t)

				local current
				if anim.is_color then
					current = interpolate_color(anim.start, anim.dest, t)
				else
					current = interpolate_num(anim.start, anim.dest, t)
				end

				widget:set_property(prop, current)

				if raw_t >= 1 then
					anims[prop] = nil
					if anim.callback then
						pcall(anim.callback, widget)
					end
				end
				widget_dirty = true
			end

			if widget_dirty then
				local root = widget
				while root.parent do root = root.parent end
				if root.form_instance then
					dirty_forms[root.form_instance] = true
				end
			end

			-- Clean empty tables
			local empty = true
			for _ in pairs(anims) do empty = false; break end
			if empty then
				modern_ui.active_animations[widget] = nil
			end
		end

		modern_ui.suppress_immediate_render = nil

		for form, _ in pairs(dirty_forms) do
			form:update()
		end
	end)
end


-- =============================================================================
-- WIDGET CORE CLASS
-- =============================================================================

local Widget = {}
Widget.__index = Widget

modern_ui.Widget = Widget

function Widget:new(type_name, props)
	local obj = {
		type = type_name or "widget",
		children = {},
		properties = {},
		bindings = {},
		unsubscribers = {},
		-- Computed geometry:
		x = 0,
		y = 0,
		width = 0,
		height = 0,
		-- Interaction states:
		hovered = false,
		focused = false,
		disabled = false,
	}
	setmetatable(obj, Widget)

	if props then
		for k, v in pairs(props) do
			if type(k) == "number" then
				if type(v) == "table" and v.type then
					table.insert(obj.children, v)
					v.parent = obj
				end
			else
				if type(v) == "table" and v.is_binding then
					obj.bindings[k] = v
					-- Subscribe for reactivity and save unsubscribes
					local unsub = v.state:subscribe(v.key, function(new_val)
						local resolved = new_val
						if v.transform then
							resolved = v.transform(new_val)
						end
						obj:set_property(k, resolved)
					end)
					table.insert(obj.unsubscribers, unsub)

					-- Initial value resolution:
					local initial = v.state[v.key]
					if v.transform then
						initial = v.transform(initial)
					end
					obj.properties[k] = initial
				else
					obj.properties[k] = v
				end
			end
		end
	end

	-- Generate a persistent ID if not provided:
	if not obj.properties.id then
		obj.properties.id = obj.type .. "_" .. tostring(math.random(100000, 999999))
	end

	return obj
end

function Widget:destroy()
	if self.unsubscribers then
		for _, unsub in ipairs(self.unsubscribers) do
			pcall(unsub)
		end
		self.unsubscribers = {}
	end
	for _, child in ipairs(self.children) do
		child:destroy()
	end
end

function Widget:set_property(key, value)
	-- Support dynamic assignment of reactive bindings inside set_property
	if type(value) == "table" and value.is_binding then
		if not self.unsubscribers then
			self.unsubscribers = {}
		end
		self.bindings[key] = value
		local unsub = value.state:subscribe(value.key, function(new_val)
			local resolved = new_val
			if value.transform then
				resolved = value.transform(new_val)
			end
			self:set_property(key, resolved)
		end)
		table.insert(self.unsubscribers, unsub)

		-- Resolve initial value
		local initial = value.state[value.key]
		if value.transform then
			initial = value.transform(initial)
		end
		value = initial
	end

	local old = self.properties[key]
	if old ~= value then
		self.properties[key] = value
		if self.on_property_changed then
			self:on_property_changed(key, value, old)
		end
		-- Re-trigger parent layouts and formspec re-generation
		if self.parent then
			self.parent:request_layout()
		elseif self.form_instance then
			if not modern_ui.suppress_immediate_render then
				self.form_instance:update()
			end
		end
	end
end

function Widget:get_property(key, default)
	local val = self.properties[key]
	if val == nil then
		return default
	end
	return val
end

function Widget:animate(prop, dest, duration, easing, callback)
	duration = duration or 0.5
	easing = easing or "linear"

	local start = self:get_property(prop)
	if start == nil then
		if prop == "opacity" or prop == "scale" then start = 1
		elseif prop == "x_offset" or prop == "y_offset" or prop == "rotation" then start = 0
		elseif prop == "background" or prop == "color" then start = "#FFFFFF"
		else start = 0
		end
	end

	local is_color = false
	if type(dest) == "string" and (dest:find("#") or prop == "background" or prop == "color") then
		is_color = true
	end

	if not modern_ui.active_animations[self] then
		modern_ui.active_animations[self] = {}
	end

	modern_ui.active_animations[self][prop] = {
		start = start,
		dest = dest,
		duration = duration,
		elapsed = 0,
		easing = easing,
		is_color = is_color,
		callback = callback
	}
end

function Widget:request_layout()
	if self.parent then
		self.parent:request_layout()
	elseif self.form_instance then
		self.form_instance:update()
	end
end

-- =============================================================================
-- CUSTOM WIDGETS REGISTRATION
-- =============================================================================

modern_ui.custom_widgets = {}

function modern_ui.register_widget(name, constructor)
	modern_ui.custom_widgets[name] = constructor
	-- Automatically register in DSL builders so they are accessible in sandboxed scopes
	modern_ui.builders[name] = function(props)
		return constructor(props)
	end
	modern_ui[name] = modern_ui.builders[name]
end


-- =============================================================================
-- LAYOUT RESOLUTION ENGINE
-- =============================================================================

-- Helper to parse padding and margin
local function parse_box_spacing(val)
	if type(val) == "number" then
		return { top = val, bottom = val, left = val, right = val }
	elseif type(val) == "table" then
		return {
			top = val.top or 0,
			bottom = val.bottom or 0,
			left = val.left or 0,
			right = val.right or 0
		}
	end
	return { top = 0, bottom = 0, left = 0, right = 0 }
end

-- Estimates a widget's intrinsic size
function Widget:measure()
	local width = self:get_property("width", "auto")
	local height = self:get_property("height", "auto")

	local measured_w = 0
	local measured_h = 0

	if type(width) == "number" then
		measured_w = width
	elseif width == "auto" then
		if self.type == "label" or self.type == "button" then
			local text = self:get_property("text", "") or self:get_property("label", "")
			measured_w = math.max(1.5, string.len(tostring(text)) * 0.15 + 0.5)
		elseif self.type == "image" or self.type == "icon" then
			measured_w = 1.5
		elseif self.type == "column" then
			local max_child_w = 0
			for _, child in ipairs(self.children) do
				local cw, _ = child:measure()
				max_child_w = math.max(max_child_w, cw)
			end
			measured_w = max_child_w
		elseif self.type == "row" then
			local sum_child_w = 0
			for _, child in ipairs(self.children) do
				local cw, _ = child:measure()
				sum_child_w = sum_child_w + cw
			end
			measured_w = sum_child_w + math.max(0, #self.children - 1) * (self:get_property("spacing", 0))
		else
			measured_w = 2.0
		end
	end

	if type(height) == "number" then
		measured_h = height
	elseif height == "auto" then
		if self.type == "label" then
			measured_h = 0.4
		elseif self.type == "button" or self.type == "checkbox" or self.type == "toggle" or self.type == "switch" then
			measured_h = 0.8
		elseif self.type == "image" or self.type == "icon" then
			measured_h = 1.5
		elseif self.type == "column" then
			local sum_child_h = 0
			for _, child in ipairs(self.children) do
				local _, ch = child:measure()
				sum_child_h = sum_child_h + ch
			end
			measured_h = sum_child_h + math.max(0, #self.children - 1) * (self:get_property("spacing", 0))
		elseif self.type == "row" then
			local max_child_h = 0
			for _, child in ipairs(self.children) do
				local _, ch = child:measure()
				max_child_h = math.max(max_child_h, ch)
			end
			measured_h = max_child_h
		else
			measured_h = 1.0
		end
	end

	-- Apply min/max constraints
	local min_w = self:get_property("min_width")
	local max_w = self:get_property("max_width")
	local min_h = self:get_property("min_height")
	local max_h = self:get_property("max_height")

	if min_w then measured_w = math.max(measured_w, min_w) end
	if max_w then measured_w = math.min(measured_w, max_w) end
	if min_h then measured_h = math.max(measured_h, min_h) end
	if max_h then measured_h = math.min(measured_h, max_h) end

	return measured_w, measured_h
end

function Widget:compute_layout(parent_w, parent_h, parent_x, parent_y)
	self.x = parent_x + (self:get_property("x_offset") or 0)
	self.y = parent_y + (self:get_property("y_offset") or 0)

	-- 1. Resolve Sizing
	local w_prop = self:get_property("width", "auto")
	local h_prop = self:get_property("height", "auto")

	if type(w_prop) == "number" then
		self.width = w_prop
	elseif type(w_prop) == "string" and w_prop:find("%%") then
		local pct = tonumber(w_prop:match("([%d%.]+)%%")) or 100
		self.width = parent_w * (pct / 100)
	else
		-- auto size
		local intrinsic_w, _ = self:measure()
		self.width = math.min(parent_w, intrinsic_w)
	end

	if type(h_prop) == "number" then
		self.height = h_prop
	elseif type(h_prop) == "string" and h_prop:find("%%") then
		local pct = tonumber(h_prop:match("([%d%.]+)%%")) or 100
		self.height = parent_h * (pct / 100)
	else
		-- auto size
		local _, intrinsic_h = self:measure()
		self.height = math.min(parent_h, intrinsic_h)
	end

	-- Apply limits
	local min_w = self:get_property("min_width")
	local max_w = self:get_property("max_width")
	local min_h = self:get_property("min_height")
	local max_h = self:get_property("max_height")
	if min_w then self.width = math.max(self.width, min_w) end
	if max_w then self.width = math.min(self.width, max_w) end
	if min_h then self.height = math.max(self.height, min_h) end
	if max_h then self.height = math.min(self.height, max_h) end

	-- 2. Spacing / Margins / Padding
	local margin = parse_box_spacing(self:get_property("margin", 0))
	local padding = parse_box_spacing(self:get_property("padding", 0))

	self.x = self.x + margin.left
	self.y = self.y + margin.top
	self.width = math.max(0.1, self.width - (margin.left + margin.right))
	self.height = math.max(0.1, self.height - (margin.top + margin.bottom))

	local content_x = self.x + padding.left
	local content_y = self.y + padding.top
	local content_w = math.max(0.1, self.width - (padding.left + padding.right))
	local content_h = math.max(0.1, self.height - (padding.top + padding.bottom))

	-- 3. Lay out children according to container type
	if #self.children == 0 then
		return
	end

	if self.type == "column" then
		local spacing = self:get_property("spacing", 0)
		local flex_total = 0
		local fixed_h_total = 0

		-- Prepass to calculate flexible spaces vs fixed spaces
		for _, child in ipairs(self.children) do
			local c_flex = child:get_property("flex", 0)
			if c_flex > 0 then
				flex_total = flex_total + c_flex
			else
				local _, ch = child:measure()
				fixed_h_total = fixed_h_total + ch
			end
		end

		local total_spacing = spacing * (#self.children - 1)
		local remaining_h = math.max(0, content_h - fixed_h_total - total_spacing)

		local current_y = content_y
		for _, child in ipairs(self.children) do
			local c_flex = child:get_property("flex", 0)
			local child_h = 0
			if c_flex > 0 then
				child_h = remaining_h * (c_flex / flex_total)
			else
				local _, ch = child:measure()
				child_h = ch
			end

			-- horizontal alignment
			local child_w = content_w
			local child_x = content_x
			local align_h = child:get_property("align_h", "stretch")
			if align_h ~= "stretch" then
				local iw, _ = child:measure()
				child_w = math.min(content_w, iw)
				if align_h == "center" then
					child_x = content_x + (content_w - child_w) / 2
				elseif align_h == "right" then
					child_x = content_x + (content_w - child_w)
				end
			end

			child:compute_layout(child_w, child_h, child_x, current_y)
			current_y = current_y + child_h + spacing
		end

	elseif self.type == "row" then
		local spacing = self:get_property("spacing", 0)
		local flex_total = 0
		local fixed_w_total = 0

		for _, child in ipairs(self.children) do
			local c_flex = child:get_property("flex", 0)
			if c_flex > 0 then
				flex_total = flex_total + c_flex
			else
				local cw, _ = child:measure()
				fixed_w_total = fixed_w_total + cw
			end
		end

		local total_spacing = spacing * (#self.children - 1)
		local remaining_w = math.max(0, content_w - fixed_w_total - total_spacing)

		local current_x = content_x
		for _, child in ipairs(self.children) do
			local c_flex = child:get_property("flex", 0)
			local child_w = 0
			if c_flex > 0 then
				child_w = remaining_w * (c_flex / flex_total)
			else
				local cw, _ = child:measure()
				child_w = cw
			end

			-- vertical alignment
			local child_h = content_h
			local child_y = content_y
			local align_v = child:get_property("align_v", "stretch")
			if align_v ~= "stretch" then
				local _, ih = child:measure()
				child_h = math.min(content_h, ih)
				if align_v == "center" then
					child_y = content_y + (content_h - child_h) / 2
				elseif align_v == "bottom" then
					child_y = content_y + (content_h - child_h)
				end
			end

			child:compute_layout(child_w, child_h, current_x, child_y)
			current_x = current_x + child_w + spacing
		end

	elseif self.type == "grid" then
		local cols = self:get_property("columns", 2)
		local spacing = self:get_property("spacing", 0)
		local rows = math.ceil(#self.children / cols)

		local col_w = (content_w - (spacing * (cols - 1))) / cols
		local row_h = (content_h - (spacing * (rows - 1))) / rows

		for i, child in ipairs(self.children) do
			local c = (i - 1) % cols
			local r = math.floor((i - 1) / cols)

			local cx = content_x + c * (col_w + spacing)
			local cy = content_y + r * (row_h + spacing)

			child:compute_layout(col_w, row_h, cx, cy)
		end

	elseif self.type == "stack" then
		for _, child in ipairs(self.children) do
			child:compute_layout(content_w, content_h, content_x, content_y)
		end

	elseif self.type == "anchor" then
		for _, child in ipairs(self.children) do
			local anchor = child:get_property("anchor", "top_left")
			local iw, ih = child:measure()
			local cw = math.min(content_w, iw)
			local ch = math.min(content_h, ih)

			local cx = content_x
			local cy = content_y

			if anchor:find("right") then
				cx = content_x + content_w - cw
			elseif anchor:find("center_h") or anchor == "center" then
				cx = content_x + (content_w - cw) / 2
			end

			if anchor:find("bottom") then
				cy = content_y + content_h - ch
			elseif anchor:find("center_v") or anchor == "center" then
				cy = content_y + (content_h - ch) / 2
			end

			child:compute_layout(cw, ch, cx, cy)
		end

	elseif self.type == "absolute" then
		for _, child in ipairs(self.children) do
			local cx = child:get_property("x", 0)
			local cy = child:get_property("y", 0)
			local cw, ch = child:measure()

			child:compute_layout(cw, ch, content_x + cx, content_y + cy)
		end
	else
		-- Default single child container
		local child = self.children[1]
		child:compute_layout(content_w, content_h, content_x, content_y)
	end
end


-- =============================================================================
-- DECLARATIVE DSL COMPILER & SANDBOX ENVIRONMENT
-- =============================================================================

modern_ui.builders = {}

local function register_dsl_builder(name, type_name)
	modern_ui.builders[name] = function(props)
		-- Support for custom widgets overrides:
		if modern_ui.custom_widgets[name] then
			return modern_ui.custom_widgets[name](props)
		end
		return Widget:new(type_name or name, props)
	end
	-- Also register in modern_ui directly
	modern_ui[name] = modern_ui.builders[name]
end

-- Layouts
register_dsl_builder("window")
register_dsl_builder("panel")
register_dsl_builder("row")
register_dsl_builder("column")
register_dsl_builder("grid")
register_dsl_builder("stack")
register_dsl_builder("anchor")
register_dsl_builder("absolute")

-- Display
register_dsl_builder("label")
register_dsl_builder("richtext")
register_dsl_builder("image")
register_dsl_builder("icon")

-- Containers
register_dsl_builder("scrollview")
register_dsl_builder("tabs")
register_dsl_builder("splitview")

-- Inputs
register_dsl_builder("button")
register_dsl_builder("checkbox")
register_dsl_builder("toggle")
register_dsl_builder("slider")
register_dsl_builder("switch")

-- Advanced
register_dsl_builder("viewport")
register_dsl_builder("model_preview")
register_dsl_builder("canvas")
register_dsl_builder("webview")
register_dsl_builder("video")

-- DSL Sandbox Builder Execution
function modern_ui.build(func)
	local env = setmetatable({}, { __index = _G })
	for k, v in pairs(modern_ui.builders) do
		env[k] = v
	end
	setfenv(func, env)
	return func()
end


-- =============================================================================
-- STYLING SYSTEM RESOLUTION
-- =============================================================================

function Widget:resolve_style()
	local active_theme = modern_ui.themes[modern_ui.current_theme] or modern_ui.themes["dark"]
	local default_widget_style = active_theme[self.type] or {}

	local resolved = {}
	-- 1. Apply Theme Defaults
	for k, v in pairs(default_widget_style) do
		resolved[k] = v
	end

	-- 2. Apply Style class
	local style_class_name = self:get_property("style")
	if style_class_name and modern_ui.styles[style_class_name] then
		for k, v in pairs(modern_ui.styles[style_class_name]) do
			resolved[k] = v
		end
	end

	-- 3. Apply Inline overrides
	local inline = self:get_property("styles")
	if inline then
		for k, v in pairs(inline) do
			resolved[k] = v
		end
	end

	-- 4. Apply State overrides (hovered, focused, disabled)
	if self.disabled and resolved.disabled then
		for k, v in pairs(resolved.disabled) do
			resolved[k] = v
		end
	elseif self.focused and resolved.focused then
		for k, v in pairs(resolved.focused) do
			resolved[k] = v
		end
	elseif self.hovered and resolved.hover then
		for k, v in pairs(resolved.hover) do
			resolved[k] = v
		end
	end

	return resolved
end


-- =============================================================================
-- FORMSPEC COMPATIBILITY PARSER
-- =============================================================================

function modern_ui.parse_compatibility_formspec(formspec_str)
	-- Creates a root window with absolute container inside
	local root = Widget:new("window", { width = 12, height = 10 })
	local container = Widget:new("absolute", { width = "100%", height = "100%" })
	table.insert(root.children, container)
	container.parent = root

	if not formspec_str or formspec_str == "" then
		return root
	end

	-- Tokenize elements: name[args]
	for tag, args_str in formspec_str:gmatch("([%w_]+)%[([^%]]*)%]") do
		local args = {}
		for arg in args_str:gmatch("([^;]*);?") do
			if arg ~= "" then
				table.insert(args, arg)
			end
		end

		if tag == "size" and #args >= 2 then
			root:set_property("width", tonumber(args[1]) or 12)
			root:set_property("height", tonumber(args[2]) or 10)

		elseif tag == "button" and #args >= 4 then
			-- button[x,y;w,h;name;label]
			local coords = {}
			for val in args[1]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(coords, num) end
			end
			local dims = {}
			for val in args[2]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(dims, num) end
			end

			if #coords >= 2 and #dims >= 2 then
				local btn = Widget:new("button", {
					x = coords[1],
					y = coords[2],
					width = dims[1],
					height = dims[2],
					id = args[3],
					text = args[4]
				})
				table.insert(container.children, btn)
				btn.parent = container
			end

		elseif tag == "label" and #args >= 2 then
			-- label[x,y;text]
			local coords = {}
			for val in args[1]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(coords, num) end
			end

			if #coords >= 2 then
				local lbl = Widget:new("label", {
					x = coords[1],
					y = coords[2],
					text = args[2]
				})
				table.insert(container.children, lbl)
				lbl.parent = container
			end

		elseif tag == "image" and #args >= 3 then
			-- image[x,y;w,h;texture]
			local coords = {}
			for val in args[1]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(coords, num) end
			end
			local dims = {}
			for val in args[2]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(dims, num) end
			end

			if #coords >= 2 and #dims >= 2 then
				local img = Widget:new("image", {
					x = coords[1],
					y = coords[2],
					width = dims[1],
					height = dims[2],
					texture = args[3]
				})
				table.insert(container.children, img)
				img.parent = container
			end

		elseif tag == "box" and #args >= 3 then
			-- box[x,y;w,h;color]
			local coords = {}
			for val in args[1]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(coords, num) end
			end
			local dims = {}
			for val in args[2]:gmatch("([^,]+)") do
				local num = tonumber(val)
				if num then table.insert(dims, num) end
			end

			if #coords >= 2 and #dims >= 2 then
				local pnl = Widget:new("panel", {
					x = coords[1],
					y = coords[2],
					width = dims[1],
					height = dims[2],
					styles = { background = args[3] }
				})
				table.insert(container.children, pnl)
				pnl.parent = container
			end
		end
	end

	return root
end


-- =============================================================================
-- FORMSPEC COMPILER AND RENDERER
-- =============================================================================

function Widget:render_to_formspec(buffer)
	buffer = buffer or {}

	local resolved_style = self:resolve_style()

	if self.type == "window" then
		table.insert(buffer, string.format("formspec_version[9]\nsize[%f,%f]\nreal_coordinates[true]\n", self.width, self.height))
		if resolved_style.background then
			table.insert(buffer, string.format("background[0,0;%f,%f;%s;true]\n", self.width, self.height, resolved_style.background))
		end

	elseif self.type == "panel" then
		local color = resolved_style.background or "#252538"
		local r = resolved_style.radius or 0
		if r > 0 then
			table.insert(buffer, string.format("style_type[box;border_radius=%d]\n", math.floor(r)))
		end
		table.insert(buffer, string.format("box[%f,%f;%f,%f;%s]\n", self.x, self.y, self.width, self.height, color))
		if r > 0 then
			table.insert(buffer, "style_type[box;border_radius=0]\n")
		end

	elseif self.type == "label" then
		local text = safe_formspec_escape(self:get_property("text", ""))
		table.insert(buffer, string.format("label[%f,%f;%s]\n", self.x, self.y + 0.35, text))

	elseif self.type == "richtext" then
		local text = safe_formspec_escape(self:get_property("text", ""))
		local id = self:get_property("id")
		table.insert(buffer, string.format("hypertext[%f,%f;%f,%f;%s;%s]\n", self.x, self.y, self.width, self.height, id, text))

	elseif self.type == "image" or self.type == "icon" then
		local texture = self:get_property("texture", "")
		table.insert(buffer, string.format("image[%f,%f;%f,%f;%s]\n", self.x, self.y, self.width, self.height, texture))

	elseif self.type == "button" then
		local id = self:get_property("id")
		local text = safe_formspec_escape(self:get_property("text", ""))
		table.insert(buffer, string.format("button[%f,%f;%f,%f;%s;%s]\n", self.x, self.y, self.width, self.height, id, text))

	elseif self.type == "checkbox" or self.type == "toggle" or self.type == "switch" then
		local id = self:get_property("id")
		local text = safe_formspec_escape(self:get_property("text", ""))
		local selected = self:get_property("selected", false) and "true" or "false"
		table.insert(buffer, string.format("checkbox[%f,%f;%s;%s;%s]\n", self.x, self.y, id, text, selected))

	elseif self.type == "slider" then
		local id = self:get_property("id")
		local val = self:get_property("value", 0)
		table.insert(buffer, string.format("scrollbar[%f,%f;%f,%f;horizontal;%s;%d]\n", self.x, self.y, self.width, self.height, id, math.floor(val)))

	elseif self.type == "model_preview" then
		local name = self:get_property("mesh", "")
		local text = self:get_property("texture", "")
		table.insert(buffer, string.format("model[%f,%f;%f,%f;model_prev;%s;%s;0,0,0;true;true;0,0]\n", self.x, self.y, self.width, self.height, name, text))

	elseif self.type == "viewport" then
		-- Render live viewport camera feed if htmlview is supported and it is requested
		local viewport_name = self:get_property("viewport_name", "main")
		if type(htmlview) == "table" and htmlview.is_supported and htmlview.is_supported() then
			local id = self:get_property("id", "vp_" .. viewport_name)
			table.insert(buffer, string.format("image[%f,%f;%f,%f;luanti-viewport://%s]\n", self.x, self.y, self.width, self.height, viewport_name))
		else
			-- Fallback to standard black frame camera placeholder
			table.insert(buffer, string.format("box[%f,%f;%f,%f;#000000]\n", self.x, self.y, self.width, self.height))
			table.insert(buffer, string.format("label[%f,%f;[Camera Viewport: %s]]\n", self.x + 0.5, self.y + 0.5, viewport_name))
		end

	elseif self.type == "canvas" then
		local draw_list = self:get_property("draw_list", {})
		for _, cmd in ipairs(draw_list) do
			if cmd.type == "rect" then
				table.insert(buffer, string.format("box[%f,%f;%f,%f;%s]\n", self.x + (cmd.x or 0), self.y + (cmd.y or 0), cmd.w or 1, cmd.h or 1, cmd.color or "#FFFFFF"))
			elseif cmd.type == "line" then
				-- Draw line as a thin box
				local x1 = self.x + (cmd.x1 or 0)
				local y1 = self.y + (cmd.y1 or 0)
				local x2 = self.x + (cmd.x2 or 0)
				local y2 = self.y + (cmd.y2 or 0)
				local thickness = cmd.thickness or 0.05
				local lx = math.min(x1, x2)
				local ly = math.min(y1, y2)
				local lw = math.max(thickness, math.abs(x2 - x1))
				local lh = math.max(thickness, math.abs(y2 - y1))
				table.insert(buffer, string.format("box[%f,%f;%f,%f;%s]\n", lx, ly, lw, lh, cmd.color or "#FFFFFF"))
			elseif cmd.type == "image" then
				table.insert(buffer, string.format("image[%f,%f;%f,%f;%s]\n", self.x + (cmd.x or 0), self.y + (cmd.y or 0), cmd.w or 1, cmd.h or 1, cmd.texture or ""))
			end
		end
	end

	-- Traverse children
	for _, child in ipairs(self.children) do
		child:render_to_formspec(buffer)
	end

	return table.concat(buffer)
end


-- =============================================================================
-- SYSTEM STATE, REGISTRY & EVENT INTERCEPTION HOOKS
-- =============================================================================

modern_ui.active_forms = {}

function modern_ui.show(playername, formname, widget_tree)
	-- Prevent memory leaks by destroying previous root and unbinding listeners
	local prev = modern_ui.active_forms[playername]
	if prev and prev.root then
		pcall(function() prev.root:destroy() end)
	end

	local form = {
		name = formname,
		playername = playername,
		root = widget_tree,
	}
	widget_tree.form_instance = form
	modern_ui.active_forms[playername] = form

	form.update = function(self)
		local root_w = self.root:get_property("width", 12)
		local root_h = self.root:get_property("height", 10)
		if type(root_w) == "string" and root_w:find("%%") then root_w = 12 end
		if type(root_h) == "string" and root_h:find("%%") then root_h = 10 end

		self.root:compute_layout(root_w, root_h, 0, 0)
		local formspec_str = self.root:render_to_formspec()
		safe_show_formspec(self.playername, self.name, formspec_str)
	end

	form:update()
	return form
end

-- Traverses widget tree to route field callbacks
local function dispatch_fields_recursive(widget, fields)
	-- Check for buttons / ID matches
	local id = widget:get_property("id")
	if id and fields[id] ~= nil then
		local val = fields[id]

		-- 1. On Click Callback
		local on_click = widget:get_property("on_click")
		if on_click and (val == "true" or val == true or type(val) == "string") then
			on_click(widget)
		end

		-- 2. On Change Callback
		local on_change = widget:get_property("on_change")
		if on_change then
			local parsed_val = val
			if val == "true" then parsed_val = true
			elseif val == "false" then parsed_val = false
			end
			on_change(widget, parsed_val)
		end
	end

	for _, child in ipairs(widget.children) do
		dispatch_fields_recursive(child, fields)
	end
end

-- Hook into Luanti server-side field receiver
if core.register_on_player_receive_fields then
	core.register_on_player_receive_fields(function(player, formname, fields)
		local pname = player:get_player_name()
		local active = modern_ui.active_forms[pname]
		if active and active.name == formname then
			if fields.quit then
				pcall(function() active.root:destroy() end)
				modern_ui.active_forms[pname] = nil
				return true
			end
			dispatch_fields_recursive(active.root, fields)
			return true
		end
		return false
	end)
end

-- Hook player disconnect to clear resources and avoid leaks
if core.register_on_leaveplayer then
	core.register_on_leaveplayer(function(player)
		local pname = player:get_player_name()
		local active = modern_ui.active_forms[pname]
		if active then
			pcall(function() active.root:destroy() end)
			modern_ui.active_forms[pname] = nil
		end
	end)
end


-- =============================================================================
-- ACCESSIBILITY HELPERS & SELF TESTS
-- =============================================================================

function modern_ui.apply_accessibility_scale(widget_tree, scale_factor)
	local function scale_rec(widget)
		local font_size = widget:get_property("font_size")
		if font_size then
			widget:set_property("font_size", font_size * scale_factor)
		end
		local h = widget:get_property("height")
		if type(h) == "number" then
			widget:set_property("height", h * scale_factor)
		end
		for _, child in ipairs(widget.children) do
			scale_rec(child)
		end
	end
	scale_rec(widget_tree)
end

function modern_ui.run_self_tests()
	local passed = true

	-- Test 1: Declarative DSL and Widget creation
	local test_widget = modern_ui.build(function()
		return window {
			width = 10,
			height = 8,
			column {
				width = "100%",
				height = "100%",
				spacing = 1,
				label { text = "Hello" },
				button { id = "play_btn", text = "Play" }
			}
		}
	end)

	if test_widget.type ~= "window" or #test_widget.children ~= 1 then
		passed = false
		print("[Modern UI Test] DSL Widget tree failed.")
	end

	-- Test 2: Layout calculations
	test_widget:compute_layout(10, 8, 0, 0)
	local col = test_widget.children[1]
	if col.width ~= 10 or col.height ~= 8 then
		passed = false
		print("[Modern UI Test] Layout propagation failed.")
	end

	-- Test 3: Style resolution
	modern_ui.set_theme("light")
	local btn = col.children[2]
	local btn_style = btn:resolve_style()
	if btn_style.background ~= "#1E66F5" then
		passed = false
		print("[Modern UI Test] Centralized styling resolution failed.")
	end

	-- Test 4: Data Binding and Reactivity
	local state = modern_ui.reactive({ score = 100 })
	local lbl = col.children[1]
	lbl:set_property("text", state:bind("score", function(val) return "Score: " .. tostring(val) end))

	if lbl:get_property("text") ~= "Score: 100" then
		passed = false
		print("[Modern UI Test] Reactive binding initialization failed.")
	end

	state.score = 250
	if lbl:get_property("text") ~= "Score: 250" then
		passed = false
		print("[Modern UI Test] Reactive notification failed.")
	end

	-- Test 5: Formspec Compatibility Parser
	local raw_formspec = "size[12,9] button[2,2;3,1;btn_ok;Confirm]"
	local parsed_tree = modern_ui.parse_compatibility_formspec(raw_formspec)
	if parsed_tree.width ~= 12 or parsed_tree.height ~= 9 then
		passed = false
		print("[Modern UI Test] Parser failed to extract window size.")
	end

	local absolute_cont = parsed_tree.children[1]
	local parsed_btn = absolute_cont.children[1]
	if not parsed_btn or parsed_btn.type ~= "button" or parsed_btn:get_property("text") ~= "Confirm" then
		passed = false
		print("[Modern UI Test] Parser failed to extract button attributes.")
	end

	-- Test 6: Animations and Canvas
	local anim_widget = modern_ui.build(function()
		return canvas {
			draw_list = { { type = "rect", x = 0, y = 0, w = 1, h = 1, color = "#FFFFFF" } }
		}
	end)
	anim_widget:animate("x_offset", 5.0, 1.0, "spring")
	if not modern_ui.active_animations[anim_widget] then
		passed = false
		print("[Modern UI Test] Animation registration self-test failed.")
	end

	return passed
end
