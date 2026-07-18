-- Unittests for Minetek Lua Animation Engine (MLAE) with Advanced Editor features

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

	-- Add keyframe using modern ID system
	local kf_id = anim:add_keyframe({
		part = "Head",
		time = 0.0,
		position = { x = 0, y = 0, z = 0 },
		rotation = { x = 0, y = 0, z = 0 },
		visibility = true,
		glow = 0.0,
	})
	assert_eq(type(kf_id), "number")

	-- Edit keyframe by ID
	anim:edit_keyframe(kf_id, { glow = 0.5 })
	local kf = anim:get_keyframe(kf_id)
	assert_eq(kf.glow, 0.5)

	-- Move keyframe by ID
	anim:move_keyframe(kf_id, 1.5)
	assert_eq(kf.time, 1.5)

	-- Remove keyframe by ID
	anim:remove_keyframe(kf_id)
	assert_eq(anim:get_keyframe(kf_id), nil)

	-- Check backward compatible track and time APIs
	local id2 = anim:add_keyframe({
		part = "Head",
		time = 1.0,
		position = { x = 0, y = 10, z = 0 },
	})
	-- edit using (part, time)
	anim:edit_keyframe("Head", 1.0, { glow = 0.8 })
	local kf2 = anim:get_keyframe(id2)
	assert_eq(kf2.glow, 0.8)

	-- move using (part, old_time, new_time)
	anim:move_keyframe("Head", 1.0, 2.0)
	assert_eq(kf2.time, 2.0)

	-- remove using (part, time)
	anim:remove_keyframe("Head", 2.0)
	assert_eq(anim:get_keyframe(id2), nil)
end)

unittests.register("MLAE_Track_Objects", function()
	local anim = core.animation.create("test_track_obj")
	local track = anim:create_track("Arm")

	assert_eq(track.name, "Arm")
	track:set_color("#FF0000")
	assert_eq(track.color, "#FF0000")

	track:set_muted(true)
	assert_eq(track.muted, true)
	track:set_muted(false)

	track:add_keyframe({
		time = 0.5,
		position = { x = 1, y = 1, z = 1 }
	})
	assert_eq(#track:get_keyframes(), 1)
end)

unittests.register("MLAE_Folders_And_Clipboard", function()
	local anim = core.animation.create("test_folders")
	anim:create_folder("Character/Arms")
	anim:move_track("LeftArm", "Character/Arms")

	local folder = anim:get_folder("Character/Arms")
	assert_eq(folder ~= nil, true)
	assert_eq(folder.tracks[1], "LeftArm")

	-- Selection and Clipboard tests
	local id1 = anim:add_keyframe({
		part = "LeftArm",
		time = 0.2,
		position = { x = 2, y = 2, z = 2 }
	})
	local track = anim:create_track("LeftArm")
	track:select_key(id1)

	local selected = anim:get_selection()
	assert_eq(#selected, 1)
	assert_eq(selected[1], id1)

	anim:copy()
	anim:clear_selection()
	assert_eq(#anim:get_selection(), 0)

	local pasted = anim:paste(1.0)
	assert_eq(#pasted, 1)
	local p_kf = anim:get_keyframe(pasted[1])
	assert_near(p_kf.time, 1.2)
end)

unittests.register("MLAE_History_And_Undo_Redo", function()
	local anim = core.animation.create("test_history")
	anim:add_keyframe({
		part = "Leg",
		time = 0.0,
		position = { x = 0, y = 0, z = 0 }
	})
	local id = anim:add_keyframe({
		part = "Leg",
		time = 1.0,
		position = { x = 10, y = 10, z = 10 }
	})

	assert_eq(#anim:get_keyframes("Leg"), 2)

	-- Undo the last action (add_keyframe)
	local ok = anim:undo()
	assert_eq(ok, true)
	assert_eq(#anim:get_keyframes("Leg"), 1)

	-- Redo
	ok = anim:redo()
	assert_eq(ok, true)
	assert_eq(#anim:get_keyframes("Leg"), 2)
end)

unittests.register("MLAE_Constraints_And_Gizmos", function()
	local mock_object = {
		set_bone_override = function(self, part, spec)
			self.overrides = self.overrides or {}
			self.overrides[part] = spec
		end,
		set_part_visible = function(self, part, vis)
			self.visibility = self.visibility or {}
			self.visibility[part] = vis
		end,
		is_valid = function(self) return true end,
		get_pos = function(self) return { x = 0, y = 0, z = 0 } end,
		get_velocity = function(self) return { x = 0, y = 0, z = 0 } end
	}

	local controller = core.animation.create_controller(mock_object)

	-- Add procedural constraints
	controller:add_constraint({
		type = "CopyPosition",
		owner = "RightHand",
		target = "LeftHand"
	})

	assert_eq(#controller:get_constraints(), 1)

	-- Play a base animation with LeftHand pose
	local anim = core.animation.create("const_anim")
	anim:add_keyframe({
		part = "LeftHand",
		time = 0.0,
		position = { x = 5, y = 5, z = 5 }
	})
	anim:add_keyframe({
		part = "RightHand",
		time = 0.0,
		position = { x = 0, y = 0, z = 0 }
	})
	controller:add_animation("base", anim)
	controller:play("base", { blend_time = 0.0 })

	controller:update(0.0)

	-- RightHand should have copied position from LeftHand (5, 5, 5)
	local rh_ov = mock_object.overrides.RightHand
	assert_near(rh_ov.position.vec.x, 5.0)
	assert_near(rh_ov.position.vec.y, 5.0)
	assert_near(rh_ov.position.vec.z, 5.0)
end)
