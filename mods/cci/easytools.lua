-- EasyTools Creation Toolkit for CCI (Mod-based)
-- Simplifies geometry creation by instantly generating normal point-draw objects.

local safe_unpack = table.unpack or unpack

cci.easytools = {}

-- Auto Shape: Rectangle (Points, Chains & CSS styling)
function cci.easytools.create_rectangle(player_name, width, height, options)
	options = options or {}
	options.type = "rectangle"
	options.style = options.style or {}
	options.style.width = width .. "px"
	options.style.height = height .. "px"

	local obj = cci.create_object(player_name, options)
	obj:add_point(0, 0)
	obj:add_point(width, 0)
	obj:add_point(width, height)
	obj:add_point(0, height)
	obj:add_chain(1, 2, 3, 4, 1) -- Closed shape chain
	return obj
end

-- Auto Shape: Rounded Rectangle
function cci.easytools.create_rounded_rectangle(player_name, width, height, radius, options)
	options = options or {}
	options.type = "rectangle"
	options.style = options.style or {}
	options.style.width = width .. "px"
	options.style.height = height .. "px"
	options.style["border-radius"] = radius .. "px"

	local obj = cci.create_object(player_name, options)
	obj:add_point(0, 0)
	obj:add_point(width, 0)
	obj:add_point(width, height)
	obj:add_point(0, height)
	obj:add_chain(1, 2, 3, 4, 1)
	return obj
end

-- Auto Shape: Circle
function cci.easytools.create_circle(player_name, radius, options)
	options = options or {}
	options.type = "circle"
	options.style = options.style or {}
	options.style.width = (radius * 2) .. "px"
	options.style.height = (radius * 2) .. "px"
	options.style["border-radius"] = "50%"

	local obj = cci.create_object(player_name, options)
	-- Generate points mathematically for the Point Draw (Chapter 2 & Chapter 5)
	local segments = 16
	for i = 0, segments - 1 do
		local angle = (i / segments) * math.PI * 2
		local px = radius + math.cos(angle) * radius
		local py = radius + math.sin(angle) * radius
		obj:add_point(px, py)
	end

	local chain = {}
	for i = 1, segments do
		table.insert(chain, i)
	end
	table.insert(chain, 1) -- close circle
	obj:add_chain(safe_unpack(chain))
	return obj
end

-- Auto Shape: Star
function cci.easytools.create_star(player_name, outer_radius, inner_radius, points_count, options)
	options = options or {}
	options.type = "star"
	options.style = options.style or {}
	options.style.width = (outer_radius * 2) .. "px"
	options.style.height = (outer_radius * 2) .. "px"

	local obj = cci.create_object(player_name, options)
	local total_points = points_count * 2
	for i = 0, total_points - 1 do
		local angle = (i / total_points) * math.PI * 2
		local r = (i % 2 == 0) and outer_radius or inner_radius
		local px = outer_radius + math.cos(angle) * r
		local py = outer_radius + math.sin(angle) * r
		obj:add_point(px, py)
	end

	local chain = {}
	for i = 1, total_points do
		table.insert(chain, i)
	end
	table.insert(chain, 1)
	obj:add_chain(safe_unpack(chain))
	return obj
end
