-- CCI Runtime & Rendering Pipeline Bridge (Built-in - Multiplayer-Safe)
-- Translates the hierarchical Lua CCI representation into HTML/CSS DOM structure per player.
-- Provides bidirectional communication between Lua and WebView for interaction.

cci.runtime = {}

-- Check if HTMLView is supported
local function is_htmlview_available()
	return type(htmlview) == "table" and type(htmlview.run) == "function"
end

-- Generate the initial HTML frame for CCI rendering
local function get_cci_html()
	return [[
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no" />
  <title>CCI Runtime</title>
  <style>
    :root {
      --safe-area-inset-top: 0px;
      --safe-area-inset-bottom: 0px;
    }
    html, body {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
      background: transparent;
      font-family: ui-sans-serif, system-ui, -apple-system, sans-serif;
      user-select: none;
      -webkit-user-select: none;
    }
    #cci-root {
      position: relative;
      width: 100%;
      height: 100%;
      overflow: hidden;
      perspective: 1000px; /* 2.5D rendering support */
      transform-style: preserve-3d;
    }
    .cci-object {
      position: absolute;
      box-sizing: border-box;
      transform-style: preserve-3d;
      transition: transform 0.1s linear, opacity 0.1s linear;
    }
    .cci-safe-area {
      overflow: hidden !important;
    }
  </style>
</head>
<body>
  <div id="cci-root"></div>
  <script>
    const root = document.getElementById('cci-root');
    const objects = {};

    function updateDOM(data) {
      // Create or update elements
      for (const objId in data) {
        const obj = data[objId];
        let el = objects[objId];
        if (!el) {
          el = document.createElement('div');
          el.id = objId;
          el.className = 'cci-object';

          // Action Attributes: Pressable (Chapter 3)
          el.addEventListener('pointerdown', (e) => {
            e.stopPropagation();
            if (window.luanti) {
              luanti.send(JSON.stringify({ event: 'press', id: objId, x: e.clientX, y: e.clientY }));
            }
          });
          el.addEventListener('pointerup', (e) => {
            e.stopPropagation();
            if (window.luanti) {
              luanti.send(JSON.stringify({ event: 'release', id: objId, x: e.clientX, y: e.clientY }));
            }
          });

          // Action Attributes: Dragging
          let isDragging = false;
          let startX, startY;
          el.addEventListener('pointerdown', (e) => {
            if (obj.draggable) {
              isDragging = true;
              startX = e.clientX;
              startY = e.clientY;
              el.setPointerCapture(e.pointerId);
              if (window.luanti) {
                luanti.send(JSON.stringify({ event: 'drag_start', id: objId, x: e.clientX, y: e.clientY }));
              }
            }
          });
          el.addEventListener('pointermove', (e) => {
            if (isDragging) {
              const dx = e.clientX - startX;
              const dy = e.clientY - startY;
              if (window.luanti) {
                luanti.send(JSON.stringify({ event: 'dragging', id: objId, dx: dx, dy: dy }));
              }
            }
          });
          el.addEventListener('pointerup', (e) => {
            if (isDragging) {
              isDragging = false;
              el.releasePointerCapture(e.pointerId);
              if (window.luanti) {
                luanti.send(JSON.stringify({ event: 'drag_end', id: objId }));
              }
            }
          });

          objects[objId] = el;
        }

        // Set visibility
        el.style.display = obj.visible ? 'block' : 'none';

        // Set Safe Area clipping (Chapter 2 & Chapter 4)
        if (obj.safe_area) {
          el.classList.add('cci-safe-area');
        } else {
          el.classList.remove('cci-safe-area');
        }

        // Apply Custom Styles & CSS directly
        for (const [k, v] of Object.entries(obj.style || {})) {
          el.style[k] = v;
        }

        // Apply spatial 2.5D Transforms (Chapter 2 & Chapter 4)
        // Position, Depth (Z), Rotation, Scale
        const zIndex = obj.layer || 0;
        el.style.zIndex = zIndex;

        let transformStr = `translate3d(${obj.transform.x}px, ${obj.transform.y}px, ${obj.transform.z}px) rotate(${obj.transform.rotation}deg) scale(${obj.transform.scale})`;
        el.style.transform = transformStr;

        // Append to parent if parent is specified and exists, otherwise to root
        if (obj.parent && objects[obj.parent]) {
          if (el.parentNode !== objects[obj.parent]) {
            objects[obj.parent].appendChild(el);
          }
        } else {
          if (el.parentNode !== root) {
            root.appendChild(el);
          }
        }
      }

      // Remove destroyed elements
      for (const objId in objects) {
        if (!data[objId]) {
          objects[objId].remove();
          delete objects[objId];
        }
      }
    }

    if (window.luanti && luanti.on_message) {
      luanti.on_message((msg) => {
        try {
          const payload = JSON.parse(msg);
          if (payload.action === 'update') {
            updateDOM(payload.data);
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

function cci.runtime.init_session(player_name)
	if not is_htmlview_available() then
		core.log("warning", "[CCI] htmlview is not supported or enabled. Headless mock mode for " .. player_name)
		return
	end

	local session = cci.get_session(player_name)

	-- Initialize htmlview displaying for this player
	htmlview.run(session.view_id, get_cci_html())
	htmlview.display(session.view_id, {
		visible = true,
		fullscreen = true,
		safe_area = true,
		drag_embed = false,
	})

	-- Handle incoming events from the renderer
	htmlview.on_message(session.view_id, function(msg)
		local data = core.parse_json(msg)
		if data and data.id then
			local obj = session.objects[data.id]
			if obj then
				if data.event == "press" then
					obj:trigger("press", data)
				elseif data.event == "release" then
					obj:trigger("release", data)
				elseif data.event == "drag_start" then
					obj:trigger("drag_start", data)
				elseif data.event == "dragging" then
					obj:trigger("dragging", data)
				elseif data.event == "drag_end" then
					obj:trigger("drag_end", data)
				end
			end
		end
	end)

	session.active = true
	core.log("action", "[CCI] Runtime successfully initialized and rendering for player " .. player_name)
end

function cci.runtime.close_session(player_name)
	local session = cci.sessions[player_name]
	if session then
		if is_htmlview_available() then
			pcall(function()
				htmlview.stop(session.view_id)
			end)
		end
		session:destroy()
	end
end

-- Automatic player session lifecycles
core.register_on_joinplayer(function(player)
	local name = player:get_player_name()
	cci.runtime.init_session(name)
end)

core.register_on_leaveplayer(function(player)
	local name = player:get_player_name()
	cci.runtime.close_session(name)
end)

-- Global Runtime Step function (Chapter 4 - Frame updates per session)
local accumulated_time = 0
core.register_globalstep(function(dtime)
	accumulated_time = accumulated_time + dtime
	-- Process updates, limit updates to 60 fps to optimize rendering performance
	if accumulated_time >= 0.016 then
		accumulated_time = 0

		for player_name, session in pairs(cci.sessions) do
			-- Check if any attributes or transforms have updated for this specific session
			if session.is_dirty and session.active then
				session.is_dirty = false

				-- Prepare data structure to send to renderer
				local payload = {
					action = "update",
					data = {}
				}

				for id, obj in pairs(session.objects) do
					-- Prepare object details
					payload.data[id] = {
						id = obj.id,
						type = obj.type,
						visible = obj.visible,
						safe_area = obj.safe_area,
						style = obj.style,
						transform = obj.transform,
						parent = obj.parent,
						layer = obj.layer,
						draggable = (obj.events["drag_start"] ~= nil or obj.events["dragging"] ~= nil)
					}
				end

				-- Send updated DOM state using send_json (or standard serialize to JSON)
				if is_htmlview_available() then
					pcall(function()
						htmlview.send_json(session.view_id, payload)
					end)
				end
			end
		end
	end
end)
