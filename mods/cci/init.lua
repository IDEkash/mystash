-- CCI Mod Dependency Exposer
-- Author: Jules
-- Exposes the engine's built-in cci namespace to sandboxed mods and registers the test command.

-- Expose to sandboxed mod-security environment
cci = core.cci or cci
_G.cci = cci

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
