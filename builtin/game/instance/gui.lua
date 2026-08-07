-- Roblox-Style GUI Engine for Luanti (Built-in)
-- Supports ScreenGui, Frame, TextLabel, ImageLabel, TextBox, with per-player HTMLView CSS rendering.

-- ScreenGui Class (Canvas container)
Instance.classes["ScreenGui"] = {
	default_properties = {
		Enabled = true,
	},
	init = function(self)
		self.Name = "ScreenGui"
	end
}

-- Frame Class
Instance.classes["Frame"] = {
	default_properties = {
		Visible = true,
		Position = "0px, 0px", -- CSS coordinates
		Size = "100px, 100px", -- CSS dimensions
		BackgroundColor = "rgba(255, 255, 255, 0.1)",
		BorderRadius = "0px",
		ClipsDescendants = false,
	},
	init = function(self)
		self.Name = "Frame"
	end
}

-- TextLabel Class
Instance.classes["TextLabel"] = {
	default_properties = {
		Visible = true,
		Position = "0px, 0px",
		Size = "100px, 40px",
		Text = "Label",
		TextColor = "#ffffff",
		TextSize = "16px",
		Font = "sans-serif",
	},
	init = function(self)
		self.Name = "TextLabel"
	end
}

-- ImageLabel Class
Instance.classes["ImageLabel"] = {
	default_properties = {
		Visible = true,
		Position = "0px, 0px",
		Size = "100px, 100px",
		Image = "", -- URL or file name
	},
	init = function(self)
		self.Name = "ImageLabel"
	end
}

-- TextBox Class
Instance.classes["TextBox"] = {
	default_properties = {
		Visible = true,
		Position = "0px, 0px",
		Size = "150px, 40px",
		Text = "",
		PlaceholderText = "Type here...",
		TextColor = "#ffffff",
		BackgroundColor = "rgba(0,0,0,0.5)",
	},
	init = function(self)
		self.Name = "TextBox"
	end
}

-- game.ui_runtime renders the entire GUI tree to the client per player via htmlview
game.ui_runtime = {
	dirty_players = {},
}

local function is_htmlview_available()
	return type(htmlview) == "table" and type(htmlview.run) == "function"
end

-- Find the owner player session name of a given GUI Instance by searching parent tree
function game.ui_runtime.find_player_of_instance(inst)
	local parent = inst.Parent
	while parent do
		if parent.Parent == game.Workspace and parent:FindFirstChild("PlayerGui") then
			return parent.Name -- Found player folder container!
		end
		parent = parent.Parent
	end
	return nil
end

function game.ui_runtime.mark_instance_dirty(inst)
	local player_name = game.ui_runtime.find_player_of_instance(inst)
	if player_name then
		game.ui_runtime.dirty_players[player_name] = true
	end
end

-- Generate a clean, modern HTML container with full CSS rendering support
local function get_gui_html()
	return [[
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no" />
  <title>Roblox GUI Environment</title>
  <style>
    html, body {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
      background: transparent;
      font-family: sans-serif;
      user-select: none;
      -webkit-user-select: none;
    }
    #gui-root {
      position: relative;
      width: 100%;
      height: 100%;
      overflow: hidden;
    }
    .gui-element {
      position: absolute;
      box-sizing: border-box;
      pointer-events: auto;
    }
    .gui-frame {
      display: block;
    }
    .gui-textlabel {
      display: flex;
      align-items: center;
      justify-content: center;
      text-align: center;
    }
    .gui-imagelabel {
      background-size: cover;
      background-position: center;
    }
    .gui-textbox {
      border: 1px solid rgba(255,255,255,0.2);
      outline: none;
      padding: 0 10px;
    }
  </style>
</head>
<body>
  <div id="gui-root"></div>
  <script>
    const root = document.getElementById('gui-root');
    const elements = {};

    function updateGUI(data) {
      for (const ref in data) {
        const item = data[ref];
        let el = elements[ref];
        if (!el) {
          if (item.ClassName === 'TextBox') {
            el = document.createElement('input');
            el.type = 'text';
            el.className = 'gui-element gui-textbox';
            el.addEventListener('input', (e) => {
              if (window.luanti) {
                luanti.send(JSON.stringify({ event: 'text_changed', ref: ref, text: el.value }));
              }
            });
          } else if (item.ClassName === 'ImageLabel') {
            el = document.createElement('div');
            el.className = 'gui-element gui-imagelabel';
          } else if (item.ClassName === 'TextLabel') {
            el = document.createElement('div');
            el.className = 'gui-element gui-textlabel';
          } else {
            el = document.createElement('div');
            el.className = 'gui-element gui-frame';
          }

          // Roblox-style input trigger (.Activated event mapping)
          el.addEventListener('pointerdown', (e) => {
            e.stopPropagation();
            if (window.luanti) {
              luanti.send(JSON.stringify({ event: 'activated', ref: ref }));
            }
          });

          elements[ref] = el;
        }

        // Apply spatial properties
        const [posX, posY] = item.Position.split(',');
        const [sizeW, sizeH] = item.Size.split(',');
        el.style.left = posX.trim();
        el.style.top = posY.trim();
        el.style.width = sizeW.trim();
        el.style.height = sizeH.trim();
        el.style.display = item.Visible ? 'flex' : 'none';

        if (item.ClassName === 'Frame') {
          el.style.backgroundColor = item.BackgroundColor;
          el.style.borderRadius = item.BorderRadius;
          el.style.overflow = item.ClipsDescendants ? 'hidden' : 'visible';
        } else if (item.ClassName === 'TextLabel') {
          el.textContent = item.Text;
          el.style.color = item.TextColor;
          el.style.fontSize = item.TextSize;
          el.style.fontFamily = item.Font;
        } else if (item.ClassName === 'ImageLabel') {
          el.style.backgroundImage = `url(${item.Image})`;
        } else if (item.ClassName === 'TextBox') {
          el.value = item.Text;
          el.placeholder = item.PlaceholderText;
          el.style.color = item.TextColor;
          el.style.backgroundColor = item.BackgroundColor;
        }

        // Append to parent if applicable
        if (item.ParentRef && elements[item.ParentRef]) {
          if (el.parentNode !== elements[item.ParentRef]) {
            elements[item.ParentRef].appendChild(el);
          }
        } else {
          if (el.parentNode !== root) {
            root.appendChild(el);
          }
        }
      }

      // Cleanup
      for (const ref in elements) {
        if (!data[ref]) {
          elements[ref].remove();
          delete elements[ref];
        }
      }
    }

    if (window.luanti && luanti.on_message) {
      luanti.on_message((msg) => {
        try {
          const payload = JSON.parse(msg);
          if (payload.action === 'render') {
            updateGUI(payload.data);
          }
        } catch(e) {
          console.error(e);
        }
      });
    }
  </script>
</body>
</html>
]]
end

-- Initialize the GUI view channel for a player
function game.ui_runtime.init_player(player_name)
	if not is_htmlview_available() then
		core.log("warning", "[GUI Engine] htmlview not supported. UI running in headless mode for " .. player_name)
		return
	end

	local view_id = "gui_view_" .. player_name
	htmlview.run(view_id, get_gui_html())
	htmlview.display(view_id, {
		visible = true,
		fullscreen = true,
		safe_area = true,
		drag_embed = false,
	})

	-- Handle bidirectional inputs and route them to Lua-side Instance events (.Activated, .TextChanged)
	htmlview.on_message(view_id, function(msg)
		local data = core.parse_json(msg)
		if data and data.ref then
			local player_entry = game.Players[player_name]
			if player_entry and player_entry.PlayerGui then
				-- Find the target element in the player's PlayerGui tree
				local function search(inst)
					local raw_inst = inst
					local mt = getmetatable(raw_inst)
					if mt and mt.__index(raw_inst, "_ref") == data.ref then
						return raw_inst
					end
					for _, child in ipairs(inst:GetChildren()) do
						local res = search(child)
						if res then return res end
					end
					return nil
				end

				local target = search(player_entry.PlayerGui)
				if target then
					if data.event == "activated" and target.Activated then
						target.Activated(target)
					elseif data.event == "text_changed" then
						target.Text = data.text
						if target.TextChanged then
							target.TextChanged(target, data.text)
						end
					end
				end
			end
		end
	end)
end

function game.ui_runtime.close_player(player_name)
	if is_htmlview_available() then
		pcall(function()
			htmlview.stop("gui_view_" .. player_name)
		end)
	end
end

-- Compile a structural representation of the player's active GUI tree
local function compile_gui_tree(player_name)
	local player_entry = game.Players[player_name]
	if not player_entry or not player_entry.PlayerGui then return nil end

	local payload = {}

	local function traverse(inst, parent_ref)
		local raw_inst = inst
		local mt = getmetatable(raw_inst)
		if not mt then return end

		local className = mt.__index(raw_inst, "ClassName")
		local ref = mt.__index(raw_inst, "_ref")

		-- We only serialize valid rendering UI classes
		if className == "ScreenGui" or className == "Frame" or className == "TextLabel" or className == "ImageLabel" or className == "TextBox" then
			if className == "ScreenGui" and not mt.__index(raw_inst, "Enabled") then
				-- ScreenGui disabled, skip rendering branch
				return
			end

			-- Build serialization packet
			local data = {
				ClassName = className,
				Visible = mt.__index(raw_inst, "Visible") ~= false,
				Position = mt.__index(raw_inst, "Position"),
				Size = mt.__index(raw_inst, "Size"),
				ParentRef = parent_ref,
			}

			if className == "Frame" then
				data.BackgroundColor = mt.__index(raw_inst, "BackgroundColor")
				data.BorderRadius = mt.__index(raw_inst, "BorderRadius")
				data.ClipsDescendants = mt.__index(raw_inst, "ClipsDescendants") == true
			elseif className == "TextLabel" then
				data.Text = mt.__index(raw_inst, "Text")
				data.TextColor = mt.__index(raw_inst, "TextColor")
				data.TextSize = mt.__index(raw_inst, "TextSize")
				data.Font = mt.__index(raw_inst, "Font")
			elseif className == "ImageLabel" then
				data.Image = mt.__index(raw_inst, "Image")
			elseif className == "TextBox" then
				data.Text = mt.__index(raw_inst, "Text")
				data.PlaceholderText = mt.__index(raw_inst, "PlaceholderText")
				data.TextColor = mt.__index(raw_inst, "TextColor")
				data.BackgroundColor = mt.__index(raw_inst, "BackgroundColor")
			end

			payload[ref] = data

			-- Render sub-children recursively
			for _, child in ipairs(inst:GetChildren()) do
				traverse(child, ref)
			end
		else
			-- If the instance is not a GUI wrapper, we still traverse its children but pass the same parent_ref down
			for _, child in ipairs(inst:GetChildren()) do
				traverse(child, parent_ref)
			end
		end
	end

	traverse(player_entry.PlayerGui, nil)
	return payload
end

-- Global high-frequency render step (capped to 60fps) to recompile and push UI DOM structures to the WebView
local accumulated_time = 0
core.register_globalstep(function(dtime)
	accumulated_time = accumulated_time + dtime
	if accumulated_time >= 0.016 then
		accumulated_time = 0

		-- Recompile and send only for dirty players
		for player_name, _ in pairs(game.ui_runtime.dirty_players) do
			game.ui_runtime.dirty_players[player_name] = nil
			local tree = compile_gui_tree(player_name)
			if tree and is_htmlview_available() then
				pcall(function()
					htmlview.send_json("gui_view_" .. player_name, {
						action = "render",
						data = tree
					})
				end)
			end
		end
	end
end)
