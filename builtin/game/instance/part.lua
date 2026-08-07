-- Part Instance implementation (Roblox-style physical Part wrapping Luanti ObjectRefs)

Instance.classes["Part"] = {
	default_properties = {
		Position = {x=0, y=0, z=0},
		Rotation = {x=0, y=0, z=0},
		Velocity = {x=0, y=0, z=0},
		Size = {x=1, y=1, z=1},
		ObjectRef = nil, -- Native Luanti ObjectRef binding
	},
	init = function(self)
		self.Name = "Part"

		-- Bind internal synchronization handlers to the private signal to guarantee immunity to collisions
		self._property_changed_signal:Connect(function(key, value, old_value)
			local ref = self.ObjectRef
			if ref then
				if key == "Position" then
					ref:set_pos(value)
				elseif key == "Velocity" then
					ref:set_velocity(value)
				elseif key == "Rotation" then
					pcall(function()
						ref:set_look_horizontal(value.y or 0)
						ref:set_look_vertical(value.x or 0)
					end)
				end
			end
		end)
	end
}
