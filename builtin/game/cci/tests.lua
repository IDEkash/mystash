-- Unit tests for Creative Composition Interface (CCI) core logic (Built-in - Multiplayer-Safe & CSS Enabled)
-- Verifies object composition, points & geometry, spatial transforms, hierarchy, events, CSS classes, and lifecycle management.

local function run_tests()
	local passed = 0
	local failed = 0
	local function assert_eq(a, b, msg)
		if a == b then
			passed = passed + 1
		else
			failed = failed + 1
			print("[CCI Test Fail] Expected " .. tostring(b) .. ", got " .. tostring(a) .. ". context: " .. (msg or ""))
		end
	end

	print("[CCI Tests] Starting core UI unit tests...")

	local player_name = "test_player"
	local session = cci.get_session(player_name)

	-- Test 1: Object Creation and Defaults (Chapter 2)
	local obj1 = cci.create_object(player_name, { id = "test_obj_1", x = 10, y = 20, class = "initial-class" })
	assert_eq(obj1.id, "test_obj_1", "Object ID registration")
	assert_eq(obj1.transform.x, 10, "Initial X position")
	assert_eq(obj1.transform.y, 20, "Initial Y position")
	assert_eq(obj1.transform.z, 0, "Initial Z depth")
	assert_eq(obj1.transform.scale, 1, "Initial scale")
	assert_eq(obj1.transform.rotation, 0, "Initial rotation")
	assert_eq(obj1.classes[1], "initial-class", "Initial class set correctly")

	-- Test 2: CSS Class Manipulation
	obj1:add_class("new-class")
	assert_eq(obj1.classes[2], "new-class", "add_class adds a class")

	-- Verify duplicate class isn't added
	obj1:add_class("new-class")
	assert_eq(#obj1.classes, 2, "add_class ignores duplicates")

	obj1:remove_class("initial-class")
	assert_eq(obj1.classes[1], "new-class", "remove_class removes class and shifts list")
	assert_eq(#obj1.classes, 1, "class list length after removal")

	obj1:set_class("override-class")
	assert_eq(obj1.classes[1], "override-class", "set_class overrides list")
	assert_eq(#obj1.classes, 1, "class list length after override")

	-- Test 3: Point & Chain geometry generation (Chapter 2)
	local pt_idx = obj1:add_point(50, 60)
	assert_eq(pt_idx, 1, "First point index")
	assert_eq(obj1.points[1].x, 50, "First point X")
	assert_eq(obj1.points[1].y, 60, "First point Y")

	local chain_idx = obj1:add_chain(1, 2, 3)
	assert_eq(chain_idx, 1, "First chain index")
	assert_eq(obj1.chains[1][2], 2, "Chain links correct")

	-- Test 4: Hierarchy and Transformations (Chapter 4)
	local parent = cci.create_object(player_name, { id = "test_parent", x = 100, y = 100 })
	local child = cci.create_object(player_name, { id = "test_child", x = 10, y = 10 })
	parent:add_child(child)

	assert_eq(child.parent, "test_parent", "Child parent-pointer set")
	assert_eq(parent.children[1], "test_child", "Parent child-list updated")

	-- Test 5: Functionable Services Events & Conditions (Chapter 1)
	local event_triggered = false
	child:on("click", function(self, data)
		event_triggered = true
		assert_eq(data.payload, "custom_data", "Event custom payload verified")
	end)

	child:trigger("click", { payload = "custom_data" })
	assert_eq(event_triggered, true, "Event trigger executes action")

	-- Test 6: Conditional Event Chaining (Chapter 1)
	local condition_evaluated = false
	local cond_action_executed = false
	child:on("click_cond", function(self, data)
		condition_evaluated = true
		return data.allowed == true
	end, function(self, data)
		cond_action_executed = true
	end)

	-- Condition fails
	child:trigger("click_cond", { allowed = false })
	assert_eq(condition_evaluated, true, "Condition gets evaluated")
	assert_eq(cond_action_executed, false, "Action skipped when condition returns false")

	-- Condition passes
	child:trigger("click_cond", { allowed = true })
	assert_eq(cond_action_executed, true, "Action executed when condition returns true")

	-- Test 7: Recursive Destruction (Chapter 4)
	local root_obj = cci.create_object(player_name, { id = "test_root" })
	local sub_obj = cci.create_object(player_name, { id = "test_sub" })
	root_obj:add_child(sub_obj)

	assert_eq(not not session.objects["test_root"], true, "Root exists before destruction")
	assert_eq(not not session.objects["test_sub"], true, "Sub-object exists before destruction")

	root_obj:destroy()
	assert_eq(session.objects["test_root"], nil, "Root deleted after destruction")
	assert_eq(session.objects["test_sub"], nil, "Sub-object recursively deleted")

	-- Clean up session
	session:destroy()

	-- Summary
	print(("[CCI Tests] Test run complete. Passed: %d, Failed: %d"):format(passed, failed))
	return failed == 0
end

-- Run unit tests on startup
core.register_on_mods_loaded(function()
	local success = run_tests()
	if not success then
		error("CCI Core Unit Tests Failed!")
	end
end)
