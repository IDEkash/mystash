// Native-feeling interaction layer for Luanti HTML UI

document.addEventListener('DOMContentLoaded', () => {
  // Constants & State
  let selectedWorldName = null;
  let worldsList = [];
  let serversList = [
    { name: "Public Creative Sandbox", address: "creative.luanti.org", port: "30000", clients: "12/50" },
    { name: "Survival Wilderness", address: "survival.luanti.org", port: "30001", clients: "8/100" },
    { name: "Development Playground", address: "127.0.0.1", port: "30000", clients: "Local" }
  ];

  // Helper selectors
  const $ = (id) => document.getElementById(id);
  const q = (sel) => document.querySelector(sel);
  const qAll = (sel) => document.querySelectorAll(sel);

  // Tab Navigation Handling
  qAll('.nav-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      // Deactivate other buttons and tabs
      qAll('.nav-btn').forEach(b => b.classList.remove('active'));
      qAll('.tab-pane').forEach(p => p.classList.remove('active'));

      // Activate selected
      btn.classList.add('active');
      const targetId = btn.getAttribute('data-target');
      $(targetId).classList.add('active');
    });
  });

  // World Rendering
  function renderWorlds() {
    const listContainer = $('world-list-items');
    listContainer.innerHTML = '';

    if (worldsList.length === 0) {
      listContainer.innerHTML = '<div class="world-card empty-state">No worlds found. Create one to begin!</div>';
      $('btn-play-world').disabled = true;
      $('btn-delete-world').disabled = true;
      return;
    }

    worldsList.forEach(w => {
      const card = document.createElement('div');
      card.className = 'world-card';
      if (w.name === selectedWorldName) {
        card.classList.add('selected');
      }

      card.innerHTML = `
        <div class="world-info">
          <span class="world-name">${escapeHTML(w.name)}</span>
          <span class="world-meta">Game: ${escapeHTML(w.gameid)}</span>
        </div>
      `;

      card.addEventListener('click', () => {
        selectedWorldName = w.name;
        renderWorlds();

        // Update detail settings
        $('btn-play-world').disabled = false;
        $('btn-delete-world').disabled = false;
        if (w.gameid) {
          $('select-game-id').value = w.gameid;
        }
      });

      listContainer.appendChild(card);
    });
  }

  // Server List Rendering
  function renderServers() {
    const serverContainer = $('server-list-items');
    serverContainer.innerHTML = '';

    serversList.forEach(s => {
      const card = document.createElement('div');
      card.className = 'server-card';
      card.innerHTML = `
        <div class="server-info">
          <span class="server-name">${escapeHTML(s.name)}</span>
          <span class="server-sub">${escapeHTML(s.address)}:${s.port}</span>
        </div>
        <span class="server-sub">${s.clients}</span>
      `;

      card.addEventListener('click', () => {
        $('server-address').value = s.address;
        $('server-port').value = s.port;

        // Visual indicator
        qAll('.server-card').forEach(sc => sc.classList.remove('selected'));
        card.classList.add('selected');
      });

      serverContainer.appendChild(card);
    });
  }

  // Escape HTML helper
  function escapeHTML(str) {
    if (!str) return '';
    return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  // Dialog Controls
  $('btn-create-world').addEventListener('click', () => {
    $('dlg-create-world-overlay').classList.add('active');
  });

  $('btn-cancel-create').addEventListener('click', () => {
    $('dlg-create-world-overlay').classList.remove('active');
  });

  $('btn-submit-create').addEventListener('click', () => {
    const nameInput = $('new-world-name');
    const name = nameInput.value.trim();
    const game = $('new-world-game').value;

    if (!name) {
      alert('World name cannot be empty.');
      return;
    }

    sendLua({
      action: 'create_world',
      name: name,
      gameid: game
    });

    nameInput.value = '';
    $('dlg-create-world-overlay').classList.remove('active');
  });

  $('btn-delete-world').addEventListener('click', () => {
    if (!selectedWorldName) return;
    if (confirm(`Are you sure you want to delete "${selectedWorldName}"? This action cannot be undone.`)) {
      sendLua({
        action: 'delete_world',
        name: selectedWorldName
      });
      selectedWorldName = null;
    }
  });

  // Range Slider Value Updates & Actions
  function setupRange(sliderId, valueId, luaKey) {
    const slider = $(sliderId);
    const valDisplay = $(valueId);

    slider.addEventListener('input', () => {
      let suffix = sliderId === 'range-sound-volume' ? '%' : '';
      valDisplay.textContent = slider.value + suffix;
    });

    slider.addEventListener('change', () => {
      sendLua({
        action: 'save_setting',
        key: luaKey,
        value: Number(slider.value)
      });
    });
  }

  setupRange('range-viewing-range', 'range-val-viewing-range', 'viewing_range');
  setupRange('range-sound-volume', 'range-val-sound-volume', 'sound_volume');

  // Toggle & Switch Inputs Save
  function setupCheckbox(id, luaKey, isBoolean = true) {
    const chk = $(id);
    chk.addEventListener('change', () => {
      sendLua({
        action: 'save_setting',
        key: luaKey,
        value: isBoolean ? chk.checked : (chk.checked ? 'true' : 'false')
      });
    });
  }

  setupCheckbox('chk-creative', 'creative_mode');
  setupCheckbox('chk-damage', 'enable_damage');
  setupCheckbox('chk-smooth-lighting', 'smooth_lighting');
  setupCheckbox('chk-enable-shaders', 'enable_shaders');
  setupCheckbox('chk-mipmapping', 'mip_map');
  setupCheckbox('chk-enable-sound', 'enable_sound');
  setupCheckbox('chk-node-highlighting', 'enable_node_highlighting');

  // Launch world trigger
  $('btn-play-world').addEventListener('click', () => {
    if (!selectedWorldName) return;
    sendLua({
      action: 'play_world',
      name: selectedWorldName,
      gameid: $('select-game-id').value,
      creative: $('chk-creative').checked,
      damage: $('chk-damage').checked
    });
  });

  // Direct connect trigger
  $('btn-connect').addEventListener('click', () => {
    const address = $('server-address').value.trim();
    const port = $('server-port').value.trim();
    const player = $('player-name').value.trim();
    const password = $('player-password').value;

    if (!address || !port || !player) {
      alert('Please fill out Address, Port, and Player Name.');
      return;
    }

    sendLua({
      action: 'connect_server',
      address: address,
      port: port,
      name: player,
      password: password
    });
  });

  // Quit application trigger
  $('btn-quit').addEventListener('click', () => {
    sendLua({ action: 'quit' });
  });

  // Lua Communication Utilities
  function sendLua(payload) {
    if (window.luanti && luanti.send) {
      luanti.send(JSON.stringify(payload));
    } else {
      console.log('Sending message to Luanti:', payload);
    }
  }

  // Handle incoming messages from Luanti Lua Environment
  if (window.luanti && luanti.on_message) {
    luanti.on_message((msg) => {
      try {
        const data = JSON.parse(msg);

        if (data.type === 'init_data') {
          // Version
          if (data.version) {
            $('engine-version').textContent = data.version;
          }

          // Worlds list
          if (data.worlds) {
            worldsList = data.worlds;
            if (worldsList.length > 0 && !selectedWorldName) {
              selectedWorldName = worldsList[0].name;
            }
            renderWorlds();
          }

          // Servers list
          if (data.servers) {
            serversList = data.servers;
            renderServers();
          }

          // Settings values
          if (data.settings) {
            const s = data.settings;
            if (s.creative_mode !== undefined) $('chk-creative').checked = s.creative_mode;
            if (s.enable_damage !== undefined) $('chk-damage').checked = s.enable_damage;
            if (s.smooth_lighting !== undefined) $('chk-smooth-lighting').checked = s.smooth_lighting;
            if (s.enable_shaders !== undefined) $('chk-enable-shaders').checked = s.enable_shaders;
            if (s.mip_map !== undefined) $('chk-mipmapping').checked = s.mip_map;
            if (s.enable_sound !== undefined) $('chk-enable-sound').checked = s.enable_sound;
            if (s.enable_node_highlighting !== undefined) $('chk-node-highlighting').checked = s.enable_node_highlighting;

            if (s.viewing_range !== undefined) {
              $('range-viewing-range').value = s.viewing_range;
              $('range-val-viewing-range').textContent = s.viewing_range;
            }
            if (s.sound_volume !== undefined) {
              $('range-sound-volume').value = s.sound_volume;
              $('range-val-sound-volume').textContent = s.sound_volume + '%';
            }
            if (s.player_name !== undefined) $('player-name').value = s.player_name;
          }
        }
      } catch (err) {
        console.error('Error parsing JSON from Luanti:', err);
      }
    });
  }

  // Bootstrap
  renderServers();
  sendLua({ action: 'request_init_data' });
});
