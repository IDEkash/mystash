-- Comprehensive Unit Tests for Roblox-Style Instance Engine (Built-in)
-- Verifies hierarchy replication, getters/setters, properties, child lookup arrays, and recursive destruction.

local function run_tests()
	local passed = 0
	local failed = 0
	local function assert_eq(a, b, msg)
		if a == b then
			passed = passed + 1
		else
			failed = failed + 1
			print("[Instance Test Fail] Expected " .. tostring(b) .. ", got " .. tostring(a) .. ". context: " .. (msg or ""))
		end
	end

	print("[Instance Tests] Starting Roblox-Style Engine unit tests...")

	-- Test 1: Basic Instance Creation and Class Property Merging
	local folder = Instance.new("Folder")
	assert_eq(folder.ClassName, "Folder", "ClassName set correctly")
	assert_eq(folder.Name, "Folder", "Default name initialized")

	-- Test 2: Custom Getters/Setters and Custom Properties (.Name, .Parent)
	folder.Name = "MyCoolFolder"
	assert_eq(folder.Name, "MyCoolFolder", "Name updated dynamically")

	-- Test 3: Hierarchy Parent-Child Replication (Setting Parent)
	local sub_folder = Instance.new("Folder")
	sub_folder.Name = "NestedFolder"
	sub_folder.Parent = folder

	assert_eq(sub_folder.Parent, folder, "Child Parent pointer verified")
	local children = folder:GetChildren()
	assert_eq(#children, 1, "Parent has 1 child in GetChildren()")
	assert_eq(children[1], sub_folder, "GetChildren index matches child instance")

	-- Test 4: Dynamic Child Lookup API (:FindFirstChild, :FindFirstChildOfClass)
	local found = folder:FindFirstChild("NestedFolder")
	assert_eq(found, sub_folder, "FindFirstChild finds child by Name")

	local found_class = folder:FindFirstChildOfClass("Folder")
	assert_eq(found_class, sub_folder, "FindFirstChildOfClass finds child by ClassName")

	-- Test 5: Re-parenting to different node
	local folder2 = Instance.new("Folder")
	folder2.Name = "SecondFolder"
	sub_folder.Parent = folder2

	assert_eq(sub_folder.Parent, folder2, "Parent pointer updated on re-parenting")
	assert_eq(#folder:GetChildren(), 0, "Old parent removes child from list")
	assert_eq(#folder2:GetChildren(), 1, "New parent registers child in list")

	-- Test 6: Event propagation (.Changed listener)
	local prop_name
	sub_folder.Changed:Connect(function(key)
		prop_name = key
	end)
	sub_folder.Name = "RenamedFolder"
	assert_eq(prop_name, "Name", "Property name passed to Changed listener")

	-- Test 7: Recursive Destruction (:Destroy)
	local child_part = Instance.new("Part")
	child_part.Name = "MyPart"
	child_part.Parent = folder2

	assert_eq(#folder2:GetChildren(), 2, "Parent has 2 children before destruction")

	folder2:Destroy()
	assert_eq(sub_folder.Parent, nil, "Sub-folder detached from destroyed parent")
	assert_eq(child_part.Parent, nil, "Child part detached from destroyed parent")

	-- Test 8: Camera Instance and Shake properties
	local cam = Instance.new("Camera")
	cam.FieldOfView = 80
	cam.Tilt = 5
	cam.ShakeAmplitude = 1.5
	cam.ShakeFrequency = 20
	cam.ShakeDuration = 2.0
	assert_eq(cam.FieldOfView, 80, "Camera FieldOfView set and verified")
	assert_eq(cam.Tilt, 5, "Camera Tilt set and verified")
	assert_eq(cam.ShakeAmplitude, 1.5, "Camera ShakeAmplitude set and verified")
	assert_eq(cam.ShakeFrequency, 20, "Camera ShakeFrequency set and verified")
	assert_eq(cam.ShakeDuration, 2.0, "Camera ShakeDuration set and verified")

	print(("[Instance Tests] Test run complete. Passed: %d, Failed: %d"):format(passed, failed))
	return failed == 0
end

-- Run unit tests on startup
core.register_on_mods_loaded(function()
	local success = run_tests()
	if not success then
		error("Roblox-Style Instance Core Unit Tests Failed!")
	end
end)
