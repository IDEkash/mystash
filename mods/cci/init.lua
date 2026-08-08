-- Creative Composition Interface (CCI) custom UI framework
-- Author: Jules
-- Exposes cci globally to all mods in the sandboxed mod-security environment.

if core.cci then
	-- Use native builtin engine implementation
	cci = core.cci
	_G.cci = cci
else
	-- Fallback implementation if loaded on an engine build without cci builtin module
	cci = {
		templates = {},
		sessions = {},
		current_points = {},
		current_connections = {},
		scale = 0.1,
		build_mode = true,
	}
	_G.cci = cci
	core.cci = cci
	minetest.cci = cci

	-- Hex color parsing & alpha formatting helpers
	local function parse_color(c)
		if not c or type(c) ~= "string" then
			return { r = 255, g = 255, b = 255, a = 255 }
		end
		local hex = c:gsub("^#", "")
		local r, g, b, a = 255, 255, 255, 255

		if #hex == 3 then
			r = tonumber(hex:sub(1,1) .. hex:sub(1,1), 16) or 255
			g = tonumber(hex:sub(2,2) .. hex:sub(2,2), 16) or 255
			b = tonumber(hex:sub(3,3) .. hex:sub(3,3), 16) or 255
		elseif #hex == 4 then
			r = tonumber(hex:sub(1,1) .. hex:sub(1,1), 16) or 255
			g = tonumber(hex:sub(2,2) .. hex:sub(2,2), 16) or 255
			b = tonumber(hex:sub(3,3) .. hex:sub(3,3), 16) or 255
			a = tonumber(hex:sub(4,4) .. hex:sub(4,4), 16) or 255
		elseif #hex == 6 then
			r = tonumber(hex:sub(1,2), 16) or 255
			g = tonumber(hex:sub(3,4), 16) or 255
			b = tonumber(hex:sub(5,6), 16) or 255
		elseif #hex >= 8 then
			r = tonumber(hex:sub(1,2), 16) or 255
			g = tonumber(hex:sub(3,4), 16) or 255
			b = tonumber(hex:sub(5,6), 16) or 255
			a = tonumber(hex:sub(7,8), 16) or 255
		else
			local lower = c:lower()
			if lower == "red" then return { r=255, g=0, b=0, a=255 }
			elseif lower == "green" then return { r=0, g=255, b=0, a=255 }
			elseif lower == "blue" then return { r=0, g=0, b=255, a=255 }
			elseif lower == "white" then return { r=255, g=255, b=255, a=255 }
			elseif lower == "black" then return { r=0, g=0, b=0, a=255 }
			end
		end
		return { r = r, g = g, b = b, a = a }
	end

	local function format_color(color_table)
		local r = math.max(0, math.min(255, math.floor(color_table.r)))
		local g = math.max(0, math.min(255, math.floor(color_table.g)))
		local b = math.max(0, math.min(255, math.floor(color_table.b)))
		local a = math.max(0, math.min(255, math.floor(color_table.a)))
		return string.format("#%02X%02X%02X%02X", r, g, b, a)
	end

	-- Object Class definition
	local Object = {}
	Object.__index = Object

	function Object.new(name, points, connections)
		local self = setmetatable({}, Object)
		self.name = name
		self.points = points or {}
		self.connections = connections or {}
		self.properties = {
			x = 0,
			y = 0,
			width = 0,
			height = 0,
			scale = 1.0,
			rotation = 0,
			transparency = 1.0,
			visible = true,
			pressable = false,
			hold = false,
			release = false,
			input = false,
			fill_color = nil,
			fill_image = nil,
			blur = nil,
			text = "",
		}
		self.state = {}
		self.animations = {}
		self.callbacks = {}
		return self
	end

	function Object:clone()
		local new_obj = Object.new(self.name, self.points, self.connections)
		for k, v in pairs(self.properties) do
			new_obj.properties[k] = v
		end
		for k, v in pairs(self.state) do
			new_obj.state[k] = v
		end
		for k, v in pairs(self.callbacks) do
			new_obj.callbacks[k] = v
		end
		return new_obj
	end

	function Object:set(prop, val)
		local old_val = self.properties[prop]
		self.properties[prop] = val

		if self.player_name and old_val ~= val then
			local sess = cci.sessions[self.player_name]
			if sess then
				sess.dirty = true
			end
		end
	end

	function Object:get(prop)
		return self.properties[prop]
	end

	function Object:set_state(name, val)
		self.state[name] = val
	end

	function Object:get_state(name)
		return self.state[name]
	end

	function Object:hide()
		self:set("visible", false)
	end

	function Object:show()
		self:set("visible", true)
	end

	function Object:destroy()
		if self.player_name then
			local sess = cci.sessions[self.player_name]
			if sess then
				sess.objects[self.name] = nil
				sess.dirty = true
			end
		else
			cci.templates[self.name] = nil
		end
	end

	-- Open Motion/Animation system
	function Object:animate(property, anim_def)
		if not anim_def or type(anim_def) ~= "table" then return end
		local to = anim_def.to
		local duration = anim_def.duration or 0.1
		local easing = anim_def.easing or "linear"

		local from = self:get(property) or 0
		local is_color = (property == "fill_color")
		if is_color then
			from = parse_color(from)
			to = parse_color(to)
		else
			from = tonumber(from) or 0
			to = tonumber(to) or 0
		end

		self.animations[property] = {
			from = from,
			to = to,
			duration = duration,
			easing = easing,
			elapsed = 0,
			is_color = is_color,
		}

		if self.player_name then
			local sess = cci.sessions[self.player_name]
			if sess then
				sess.dirty = true
			end
		end
	end

	-- Block A Builders
	function cci.add_point(x, y)
		local pt = { x = x, y = y, id = #cci.current_points + 1 }
		table.insert(cci.current_points, pt)
		return pt
	end

	function cci.connect(p1, p2)
		table.insert(cci.current_connections, { p1 = p1, p2 = p2 })
	end

	function cci.close(name)
		if #cci.current_points == 0 then
			return nil
		end

		-- Calculate bounding box
		local min_x, max_x = math.huge, -math.huge
		local min_y, max_y = math.huge, -math.huge
		for _, p in ipairs(cci.current_points) do
			if p.x < min_x then min_x = p.x end
			if p.x > max_x then max_x = p.x end
			if p.y < min_y then min_y = p.y end
			if p.y > max_y then max_y = p.y end
		end

		local width = max_x - min_x
		local height = max_y - min_y

		-- Relative points
		local rel_points = {}
		for _, p in ipairs(cci.current_points) do
			rel_points[p.id] = { x = p.x - min_x, y = p.y - min_y }
		end

		-- Relative connections
		local rel_conns = {}
		for _, conn in ipairs(cci.current_connections) do
			table.insert(rel_conns, { p1_id = conn.p1.id, p2_id = conn.p2.id })
		end

		local obj = Object.new(name, rel_points, rel_conns)
		obj:set("x", min_x)
		obj:set("y", min_y)
		obj:set("width", width)
		obj:set("height", height)

		cci.current_points = {}
		cci.current_connections = {}

		cci.templates[name] = obj
		return obj
	end

	-- Block B, C, D APIs
	function cci.get(name, player_name)
		if player_name then
			local sess = cci.sessions[player_name]
			if sess and sess.objects[name] then
				return sess.objects[name]
			end
		end
		return cci.templates[name]
	end

	function cci.on(obj, event, callback)
		if not obj then return end
		obj.callbacks[event] = callback
	end

	-- Session management
	function cci.show(player_name)
		local player = minetest.get_player_by_name(player_name)
		if not player then return end

		local objects = {}
		for name, template in pairs(cci.templates) do
			local clone = template:clone()
			clone.player_name = player_name
			objects[name] = clone
		end

		cci.sessions[player_name] = {
			objects = objects,
			active = true,
			dirty = true,
		}

		cci.render(player_name)
	end

	function cci.hide(player_name)
		local sess = cci.sessions[player_name]
		if sess then
			sess.active = false
		end
		minetest.close_formspec(player_name, "cci:form")
	end

	-- Line drawing interpolation
	local function draw_line(x1, y1, x2, y2, color, fs_elements)
		local dx = x2 - x1
		local dy = y2 - y1
		local dist = math.sqrt(dx*dx + dy*dy)
		local step = 0.12
		local steps = math.max(1, math.floor(dist / step))
		for i = 0, steps do
			local t = i / steps
			local px = x1 + dx * t
			local py = y1 + dy * t
			table.insert(fs_elements, string.format("box[%f,%f;0.06,0.06;%s]", px - 0.03, py - 0.03, color))
		end
	end

	-- Runtime rendering engine
	function cci.render(player_name)
		local sess = cci.sessions[player_name]
		if not sess or not sess.active then return end

		local max_x = 10
		local max_y = 8
		local render_queue = {}

		for _, obj in pairs(sess.objects) do
			if obj:get("visible") then
				table.insert(render_queue, obj)
				local ex = (obj:get("x") or 0) + (obj:get("width") or 0) * (obj:get("scale") or 1.0)
				local ey = (obj:get("y") or 0) + (obj:get("height") or 0) * (obj:get("scale") or 1.0)
				ex = ex * cci.scale
				ey = ey * cci.scale
				if ex > max_x then max_x = ex end
				if ey > max_y then max_y = ey end
			end
		end

		local size_x = max_x + 1.5
		local size_y = max_y + 1.5

		local fs = {
			"formspec_version[6]",
			string.format("size[%f,%f]", size_x, size_y),
			"no_prepends[]",
			"background[0,0;0,0;#00000055;true]"
		}

		for _, obj in ipairs(render_queue) do
			local x = (obj:get("x") or 0) * cci.scale
			local y = (obj:get("y") or 0) * cci.scale
			local scale = obj:get("scale") or 1.0
			local w = (obj:get("width") or 0) * scale * cci.scale
			local h = (obj:get("height") or 0) * scale * cci.scale
			local fill_color = obj:get("fill_color")
			local fill_image = obj:get("fill_image")
			local transparency = obj:get("transparency") or 1.0

			local is_solid = (fill_color ~= nil or fill_image ~= nil)

			if fill_color then
				local parsed = parse_color(fill_color)
				parsed.a = parsed.a * transparency
				local final_color = format_color(parsed)
				table.insert(fs, string.format("box[%f,%f;%f,%f;%s]", x, y, w, h, final_color))
			elseif fill_image then
				local tex = fill_image
				if transparency < 1.0 then
					local alpha = math.max(0, math.min(255, math.floor(transparency * 255)))
					tex = tex .. "^[opacity:" .. alpha
				end
				table.insert(fs, string.format("image[%f,%f;%f,%f;%s]", x, y, w, h, tex))
			end

			if not is_solid or cci.build_mode then
				local color = "#FFFF00"
				if is_solid then
					color = "#FFFFFF"
				end
				for _, conn in ipairs(obj.connections) do
					local p1 = obj.points[conn.p1_id]
					local p2 = obj.points[conn.p2_id]
					if p1 and p2 then
						local ax1 = x + p1.x * scale * cci.scale
						local ay1 = y + p1.y * scale * cci.scale
						local ax2 = x + p2.x * scale * cci.scale
						local ay2 = y + p2.y * scale * cci.scale
						draw_line(ax1, ay1, ax2, ay2, color, fs)
					end
				end
			end

			if cci.build_mode then
				for _, pt in pairs(obj.points) do
					local px = x + pt.x * scale * cci.scale
					local py = y + pt.y * scale * cci.scale
					table.insert(fs, string.format("box[%f,%f;0.08,0.08;#FF0000]", px - 0.04, py - 0.04))
				end
			end

			if obj:get("input") then
				local field_name = "cci_field_" .. obj.name
				table.insert(fs, string.format("field[%f,%f;%f,%f;%s;;%s]", x, y, w, h, field_name, minetest.formspec_escape(obj:get("text") or "")))
			elseif obj:get("pressable") or obj:get("hold") or obj:get("release") then
				local btn_name = "cci_btn_" .. obj.name
				table.insert(fs, string.format("style[%s;border=false;bgimg=;bgimg_pressed=;bgimg_hovered=;alpha=true]", btn_name))
				table.insert(fs, string.format("image_button[%f,%f;%f,%f;;%s;;false;false;]", x, y, w, h, btn_name))
			end
		end

		if cci.build_mode and #cci.current_points > 0 then
			for _, pt in ipairs(cci.current_points) do
				local px = pt.x * cci.scale
				local py = pt.y * cci.scale
				table.insert(fs, string.format("box[%f,%f;0.1,0.1;#FF00FF]", px - 0.05, py - 0.05))
			end
			for _, conn in ipairs(cci.current_connections) do
				local px1 = conn.p1.x * cci.scale
				local py1 = conn.p1.y * cci.scale
				local px2 = conn.p2.x * cci.scale
				local py2 = conn.p2.y * cci.scale
				draw_line(px1, py1, px2, py2, "#00FFFF", fs)
			end
		end

		sess.dirty = false
		minetest.show_formspec(player_name, "cci:form", table.concat(fs, ""))
	end

	-- Interpolation & globalstep ticker
	local function lerp(a, b, t)
		return a + (b - a) * t
	end

	local function ease(t, mode)
		if mode == "sine" or mode == "smoothstep" then
			return t * t * (3 - 2 * t)
		end
		return t
	end

	minetest.register_globalstep(function(dtime)
		for pname, sess in pairs(cci.sessions) do
			if sess.active then
				local animated = false
				for _, obj in pairs(sess.objects) do
					for prop, anim in pairs(obj.animations) do
						anim.elapsed = anim.elapsed + dtime
						local t = anim.elapsed / anim.duration
						if t >= 1 then
							if anim.is_color then
								obj.properties[prop] = format_color(anim.to)
							else
								obj.properties[prop] = anim.to
							end
							obj.animations[prop] = nil
							animated = true
						else
							local factor = ease(t, anim.easing)
							if anim.is_color then
								local r = lerp(anim.from.r, anim.to.r, factor)
								local g = lerp(anim.from.g, anim.to.g, factor)
								local b = lerp(anim.from.b, anim.to.b, factor)
								local a = lerp(anim.from.a, anim.to.a, factor)
								obj.properties[prop] = format_color({r=r, g=g, b=b, a=a})
							else
								obj.properties[prop] = lerp(anim.from, anim.to, factor)
							end
							animated = true
						end
					end
				end
				if animated then
					sess.dirty = true
				end
				if sess.dirty then
					cci.render(pname)
				end
			end
		end
	end)

	-- Formspec field receiver
	minetest.register_on_player_receive_fields(function(player, formname, fields)
		if formname ~= "cci:form" then return false end
		local pname = player:get_player_name()
		local sess = cci.sessions[pname]
		if not sess or not sess.active then return false end

		for field_name, value in pairs(fields) do
			if field_name:sub(1, 8) == "cci_btn_" then
				local name = field_name:sub(9)
				local obj = sess.objects[name]
				if obj then
					if obj.callbacks.press then
						obj.callbacks.press(obj)
					end
					if obj.callbacks.hold then
						obj.callbacks.hold(obj)
					end
					if obj.callbacks.release then
						obj.callbacks.release(obj)
					end
					sess.dirty = true
				end
			elseif field_name:sub(1, 10) == "cci_field_" then
				local name = field_name:sub(11)
				local obj = sess.objects[name]
				if obj then
					local old_text = obj:get("text") or ""
					if old_text ~= value then
						obj:set("text", value)
						if obj.callbacks.text_changed then
							obj.callbacks.text_changed(obj, value)
						end
						sess.dirty = true
					end
				end
			end
		end

		if fields.quit then
			sess.active = false
		end

		if sess.dirty then
			cci.render(pname)
		end

		return true
	end)

	-- Clean up player sessions upon disconnection
	minetest.register_on_leaveplayer(function(player)
		local pname = player:get_player_name()
		cci.sessions[pname] = nil
	end)
end

-- CCI Demonstration command for testing
minetest.register_chatcommand("cci_test", {
	params = "",
	description = "Launches the Creative Composition Interface (CCI) Test GUI",
	privs = { interact = true },
	func = function(name)
		-- BLOCK A (Geometry)
		local p1 = cci.add_point(20,20)
		local p2 = cci.add_point(120,20)
		local p3 = cci.add_point(120,60)
		local p4 = cci.add_point(20,60)
		cci.connect(p1,p2)
		cci.connect(p2,p3)
		cci.connect(p3,p4)
		cci.connect(p4,p1)
		cci.close("play_button")

		local c1 = cci.add_point(20, 80)
		local c2 = cci.add_point(50, 80)
		local c3 = cci.add_point(50, 110)
		local c4 = cci.add_point(20, 110)
		cci.connect(c1, c2)
		cci.connect(c2, c3)
		cci.connect(c3, c4)
		cci.connect(c4, c1)
		cci.close("checkbox_btn")

		local t1 = cci.add_point(70, 80)
		local t2 = cci.add_point(220, 80)
		local t3 = cci.add_point(220, 110)
		local t4 = cci.add_point(70, 110)
		cci.connect(t1, t2)
		cci.connect(t2, t3)
		cci.connect(t3, t4)
		cci.connect(t4, t1)
		cci.close("text_input")

		-- BLOCK B (Attributes + Logic)
		local btn = cci.get("play_button")
		btn:set("fill_color", "#4CAF50")
		btn:set("pressable", true)

		local chk = cci.get("checkbox_btn")
		chk:set("fill_color", "#777777")
		chk:set("pressable", true)
		chk:set_state("checked", false)

		local txt = cci.get("text_input")
		txt:set("input", true)
		txt:set("text", "Edit me!")

		-- Event bindings (Functionable Services)
		cci.on(btn, "press", function(self)
			self:animate("scale", {to = 0.9, duration = 0.1, easing = "sine"})
			minetest.after(0.15, function()
				self:animate("scale", {to = 1.0, duration = 0.1, easing = "sine"})
			end)
		end)

		cci.on(chk, "press", function(self)
			if self:get_state("checked") then
				self:set_state("checked", false)
				self:animate("fill_color", {to = "#777777", duration = 0.2, easing = "linear"})
			else
				self:set_state("checked", true)
				self:animate("fill_color", {to = "#4CAF50", duration = 0.2, easing = "linear"})
			end
		end)

		cci.on(txt, "text_changed", function(self, new_val)
			minetest.chat_send_player(name, "Text updated: " .. new_val)
		end)

		cci.show(name)
		return true, "CCI Demonstration UI loaded successfully!"
	end,
})
