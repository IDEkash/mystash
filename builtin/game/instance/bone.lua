-- BoneTransform Instance implementation (Roblox-style skeletal bone controller)

Instance.classes["BoneTransform"] = {
	default_properties = {
		BoneName = "",
		Position = {x=0, y=0, z=0},
		Rotation = {x=0, y=0, z=0},
		Scale = {x=1, y=1, z=1},
		Visible = true,
	},
	init = function(self)
		self.Name = "BoneTransform"

		-- Propagate changes to the native bone skeletal overrides using private sync signal
		self._property_changed_signal:Connect(function(key, value)
			if self.BoneName == "" then return end

			-- Traversal up the hierarchy to find the bounding physical Part
			local parent = self.Parent
			while parent and parent.ClassName ~= "Part" do
				parent = parent.Parent
			end

			if parent and parent.ObjectRef then
				local ref = parent.ObjectRef
				if key == "Position" then
					pcall(function() ref:set_bone_position(self.BoneName, value) end)
				elseif key == "Rotation" then
					pcall(function() ref:set_bone_rotation(self.BoneName, value) end)
				elseif key == "Scale" then
					pcall(function() ref:set_bone_scale(self.BoneName, value) end)
				elseif key == "Visible" then
					pcall(function() ref:set_part_visible(self.BoneName, value) end)
				end
			end
		end)
	end
}
