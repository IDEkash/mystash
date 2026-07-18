-- Unittests for Minetek Lua Animation Engine (MLAE)

local function assert_eq(a, b, msg)
	if a ~= b then
		error(string.format("Expected %s, got %s. %s", tostring(b), tostring(a), msg or ""))
	end
end

local function assert_near(a, b, epsilon, msg)
	if math.abs(a - b) > (epsilon or 1e-5) then
		error(string.format("Expected %s to be near %s. %s", tostring(a), tostring(b), msg or ""))
	end
end

unittests.register("MLAE_Easing_And_Curves", function()
	-- 1. Test Easings
	local linear = core.animation.easings.linear
	assert_eq(linear(0.5), 0.5)

	local smoothstep = core.animation.easings.smoothstep
	assert_eq(smoothstep(0), 0)
	assert_eq(smoothstep(1), 1)
	assert_near(smoothstep(0.5), 0.5)
	assert_near(smoothstep(0.25), 0.15625)

	local sine_in = core.animation.easings.sine_in
	assert_eq(sine_in(0), 0)
	assert_near(sine_in(0.5), 0.29289)

	local bounce_out = core.animation.easings.bounce_out
	assert_eq(bounce_out(1), 1)

	-- Register custom curve
	core.animation.register_easing("custom_double", function(t)
		return t * 2
	end)
	local custom = core.animation.easings.custom_double
	assert_eq(custom(0.3), 0.6)
end)

unittests.register("MLAE_Animation_Creation_And_Editing", function()
	local anim = core.animation.create("test_anim")
	assert_eq(anim:get_length(), 1.0)
	anim:set_length(2.5)
	assert_eq(anim:get_length(), 2.5)

	-- Add keyframes
	anim:add_keyframe({
		part = "Head",
		time = 0.0,
		position = { x = 0, y = 0, z = 0 },
		rotation = { x = 0, y = 0, z = 0 },
		visibility = true,
		glow = 0.0,
	})

	anim:add_keyframe({
		part = "Head",
		time = 1.0,
		position = { x = 0, y = 10, z = 0 },
		rotation = { x = 90, y = 0, z = 0 },
		visibility = false,
		glow = 1.0,
		easing = "sine_out"
	})

	assert_eq(#anim:get_tracks(), 1)
	assert_eq(anim:get_tracks()[1], "Head")

	local kfs = anim:get_keyframes("Head")
	assert_eq(#kfs, 2)
	assert_eq(kfs[1].time, 0.0)
	assert_eq(kfs[2].time, 1.0)

	-- Edit keyframe
	anim:edit_keyframe("Head", 1.0, { glow = 0.5 })
	kfs = anim:get_keyframes("Head")
	assert_eq(kfs[2].glow, 0.5)

	-- Move keyframe
	anim:move_keyframe("Head", 1.0, 1.5)
	kfs = anim:get_keyframes("Head")
	assert_eq(kfs[2].time, 1.5)

	-- Remove keyframe
	anim:remove_keyframe("Head", 1.5)
	kfs = anim:get_keyframes("Head")
	assert_eq(#kfs, 1)

	-- Create track
	anim:create_track("LeftArm")
	assert_eq(#anim:get_tracks(), 2)

	-- Rename track
	anim:rename_track("LeftArm", "RightArm")
	local tracks = anim:get_tracks()
	assert_eq(tracks[1], "Head")
	assert_eq(tracks[2], "RightArm")
end)

unittests.register("MLAE_Serialization_And_Cloning", function()
	local anim = core.animation.create("wave")
	anim:set_length(5.0)
	anim:add_keyframe({
		part = "Hand",
		time = 0.5,
		position = { x = 1, y = 2, z = 3 },
		easing = "bounce_out"
	})
	anim:add_marker(1.2, "Footstep", "left")

	-- JSON Serialization
	local json_str = anim:serialize("json")
	assert_eq(type(json_str), "string")

	local anim2 = core.animation.deserialize(json_str)
	assert_eq(anim2.name, "wave")
	assert_eq(anim2:get_length(), 5.0)
	local kfs = anim2:get_keyframes("Hand")
	assert_eq(#kfs, 1)
	assert_eq(kfs[1].time, 0.5)
	assert_eq(kfs[1].position.x, 1)
	assert_eq(kfs[1].position.y, 2)
	assert_eq(kfs[1].position.z, 3)
	assert_eq(#anim2:get_markers(), 1)
	assert_eq(anim2:get_markers()[1].name, "Footstep")

	-- Compressed Serialization
	local comp_str = anim:serialize("compressed")
	local anim3 = core.animation.deserialize(comp_str)
	assert_eq(anim3.name, "wave")
	assert_eq(anim3:get_length(), 5.0)

	-- Cloning
	local cloned = anim:clone()
	assert_eq(cloned.name, "wave")
end)

unittests.register("MLAE_Controller_And_Blending", function()
	-- Create a mock objectref
	local mock_object = {
		set_bone_override = function(self, part, spec)
			self.overrides = self.overrides or {}
			self.overrides[part] = spec
		end,
		set_part_visible = function(self, part, vis)
			self.visibility = self.visibility or {}
			self.visibility[part] = vis
		end,
		is_valid = function(self)
			return true
		end,
		get_pos = function(self)
			return { x = 0, y = 0, z = 0 }
		end,
		set_pos = function(self, pos)
			self.pos = pos
		end,
		get_velocity = function(self)
			return { x = 10, y = 0, z = 0 }
		end
	}

	local controller = core.animation.create_controller(mock_object)
	assert_eq(type(controller), "table")

	local anim1 = core.animation.create("anim1")
	anim1:set_length(2.0)
	anim1:add_keyframe({
		part = "Torso",
		time = 0.0,
		position = { x = 0, y = 0, z = 0 },
		rotation = { x = 0, y = 0, z = 0 },
	})
	anim1:add_keyframe({
		part = "Torso",
		time = 2.0,
		position = { x = 10, y = 0, z = 0 },
		rotation = { x = 40, y = 0, z = 0 },
	})

	controller:add_animation("walk", anim1)
	controller:play("walk", { blend_time = 0.0, speed = 1.0, loop = true })

	-- Update controller
	controller:update(1.0) -- Go to t = 1.0 (halfway)
	assert_eq(controller:is_playing("walk"), true)
	assert_near(controller:get_animation_time("walk"), 1.0)

	-- Check bone overrides
	local torso_override = mock_object.overrides.Torso
	assert_eq(torso_override ~= nil, true)
	assert_near(torso_override.position.vec.x, 5.0)
	assert_near(torso_override.rotation.vec.x, 20.0)

	-- Test queue
	controller:enqueue("walk", { speed = 2.0 })
	assert_eq(#controller.queue, 1)
	assert_eq(controller:peek_queue().name, "walk")
	local dequeued = controller:dequeue()
	assert_eq(dequeued.name, "walk")
	assert_eq(dequeued.opts.speed, 2.0)

	-- Test State Machine
	controller:add_state("Idle", { animation = "walk", speed = 0.5, loop = true })
	controller:add_state("Moving", { animation = "walk", speed = 1.5, loop = true })
	controller:set_state("Idle")
	assert_eq(controller:get_state(), "Idle")

	-- Test curves
	local anim2 = core.animation.create("glow_curve")
	anim2:set_length(1.0)
	anim2:create_curve("GlowVal")
	anim2:add_curve_keyframe("GlowVal", 0.0, 0.0)
	anim2:add_curve_keyframe("GlowVal", 1.0, 10.0)
	controller:add_animation("glowy", anim2)
	controller:play("glowy", { blend_time = 0.0, loop = false })

	controller:update(0.5)
	assert_near(controller:get_curve_value("GlowVal"), 5.0)

	-- Test layers override/additive blending
	local anim_add = core.animation.create("add_anim")
	anim_add:set_length(1.0)
	anim_add:add_keyframe({
		part = "Torso",
		time = 0.0,
		position = { x = 5, y = 5, z = 5 }
	})
	controller:add_animation("add_pose", anim_add)
	controller:play("add_pose", { layer = "Additive" })

	controller:update(0.0)
	-- Since "Additive" priority is higher and uses additive blend, the Torso position should add 5.0
	local torso_ov = mock_object.overrides.Torso
	assert_near(torso_ov.position.vec.x, 5.0) -- wait: anim1 was walk but in State Machine set_state("Idle") it replayed walk at t=0.
	-- So base torso is 0.0, additive layer adds 5.0, sum is 5.0. That's correct!
end)
