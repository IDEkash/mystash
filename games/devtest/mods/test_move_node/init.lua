core.register_node("test_move_node:elevator", {
	description = "Movable Elevator Platform Node",
	tiles = {"default_stone.png"},
	groups = {cracky = 3, movable = 1}, -- triggers group-based defaults

	on_punch = function(pos, node, puncher, pointed_thing)
		if not puncher or not puncher:is_player() then return end
		local name = puncher:get_player_name()

		local target_pos = vector.add(pos, {x=0, y=5, z=0})
		local orig_pos = vector.new(pos)

		core.chat_send_player(name, "Activating Single Elevator... Hold on!")

		core.move_node(pos, {
			to = target_pos,
			time = 3.0,
			easing = "smoothstep",
			collide = true,
			on_complete = function(h)
				core.chat_send_player(name, "Elevator reached the top! Returning in 2 seconds...")
				core.after(2.0, function()
					core.move_node(target_pos, {
						to = orig_pos,
						time = 3.0,
						easing = "smoothstep",
						collide = true,
						on_complete = function(h2)
							core.chat_send_player(name, "Elevator returned to bottom!")
						end
					})
				end)
			end
		})
	end
})

core.register_node("test_move_node:elevator_multipart_trigger", {
	description = "Movable Multi-part Elevator Trigger",
	tiles = {"default_cobble.png"},
	groups = {cracky = 3},

	on_punch = function(pos, node, puncher, pointed_thing)
		if not puncher or not puncher:is_player() then return end
		local name = puncher:get_player_name()

		-- We define a 3x1x3 region centered horizontally, offset by 1 in Z
		local min_pos = vector.add(pos, {x=-1, y=0, z=1})
		local max_pos = vector.add(pos, {x=1, y=0, z=3})

		-- Place elevator nodes in the region first so we can move them
		for x = min_pos.x, max_pos.x do
			for z = min_pos.z, max_pos.z do
				core.set_node({x=x, y=min_pos.y, z=z}, {name="test_move_node:elevator"})
			end
		end

		core.chat_send_player(name, "Activating 3x3 Multi-part Elevator! Hold on!")

		local target_min = vector.add(min_pos, {x=0, y=5, z=0})
		local target_max = vector.add(max_pos, {x=0, y=5, z=0})

		core.move_node({min = min_pos, max = max_pos}, {
			to = target_min,
			time = 3.0,
			easing = "smoothstep",
			collide = true,
			on_complete = function(h)
				core.chat_send_player(name, "3x3 Elevator reached the top! Returning in 2 seconds...")
				core.after(2.0, function()
					core.move_node({min = target_min, max = target_max}, {
						to = min_pos,
						time = 3.0,
						easing = "smoothstep",
						collide = true,
						on_complete = function(h2)
							core.chat_send_player(name, "3x3 Elevator returned to bottom!")
						end
					})
				end)
			end
		})
	end
})
