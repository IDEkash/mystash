-- Camera Instance implementation (Roblox-style player viewport camera controller)

Instance.classes["Camera"] = {
	default_properties = {
		FieldOfView = 70,
		Mode = "firstperson",
		Smooth = false,
		Tilt = 0,
		ShakeAmplitude = 0,
		ShakeFrequency = 0,
		ShakeDuration = 0,
		PlayerRef = nil, -- Direct Player ObjectRef binding
	},
	init = function(self)
		self.Name = "Camera"

		-- Hook state changes to native client-side Camera APIs using private sync signal
		self._property_changed_signal:Connect(function(key, value)
			local player = self.PlayerRef
			if player then
				if key == "FieldOfView" then
					pcall(function()
						player:set_fov(value)
					end)
				elseif key == "Mode" or key == "Smooth" or key == "Tilt" or key == "ShakeAmplitude" or key == "ShakeFrequency" or key == "ShakeDuration" then
					pcall(function()
						player:set_camera({
							mode = self.Mode,
							smooth = self.Smooth,
							tilt = self.Tilt,
							shake_amplitude = self.ShakeAmplitude,
							shake_frequency = self.ShakeFrequency,
							shake_timer = self.ShakeDuration,
						})
					end)
				end
			end
		end)
	end
}
