{
	NO_MAP_LOCK_REQUIRED;
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	SkyboxParams sky_params = player->getSkyParams();

	// reset if empty
	if (lua_isnoneornil(L, 2) && lua_isnone(L, 3)) {
		sky_params = SkyboxDefaults::getSkyDefaults();
	} else if (lua_istable(L, 2) && !is_color_table(L, 2)) {
		lua_getfield(L, 2, "base_color");
		if (!lua_isnil(L, -1))
			read_color(L, -1, &sky_params.bgcolor);
		lua_pop(L, 1);

		lua_getfield(L, 2, "body_orbit_tilt");
		if (!lua_isnil(L, -1)) {
			sky_params.body_orbit_tilt = rangelim(readParam<float>(L, -1), -60.0f, 60.0f);
		}
		lua_pop(L, 1);

		lua_getfield(L, 2, "type");
		if (!lua_isnil(L, -1))
			sky_params.type = luaL_checkstring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "textures");
		sky_params.textures.clear();
		if (lua_istable(L, -1) && sky_params.type == "skybox") {
			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				// Key is at index -2 and value at index -1
				sky_params.textures.emplace_back(readParam<std::string>(L, -1));
				// Removes the value, but keeps the key for iteration
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		// Validate that we either have six or zero textures
		if (sky_params.textures.size() != 6 && !sky_params.textures.empty())
			throw LuaError("Skybox expects 6 textures!");

		sky_params.clouds = getboolfield_default(L, 2, "clouds", sky_params.clouds);

		lua_getfield(L, 2, "sky_color");
		if (lua_istable(L, -1)) {
			lua_getfield(L, -1, "day_sky");
			read_color(L, -1, &sky_params.sky_color.day_sky);
			lua_pop(L, 1);

			lua_getfield(L, -1, "day_horizon");
			read_color(L, -1, &sky_params.sky_color.day_horizon);
			lua_pop(L, 1);

			lua_getfield(L, -1, "dawn_sky");
			read_color(L, -1, &sky_params.sky_color.dawn_sky);
			lua_pop(L, 1);

			lua_getfield(L, -1, "dawn_horizon");
			read_color(L, -1, &sky_params.sky_color.dawn_horizon);
			lua_pop(L, 1);

			lua_getfield(L, -1, "night_sky");
			read_color(L, -1, &sky_params.sky_color.night_sky);
			lua_pop(L, 1);

			lua_getfield(L, -1, "night_horizon");
			read_color(L, -1, &sky_params.sky_color.night_horizon);
			lua_pop(L, 1);

			lua_getfield(L, -1, "indoors");
			read_color(L, -1, &sky_params.sky_color.indoors);
			lua_pop(L, 1);

			// Prevent flickering clouds at dawn/dusk:
			sky_params.fog_sun_tint = video::SColor(255, 255, 255, 255);
			lua_getfield(L, -1, "fog_sun_tint");
			read_color(L, -1, &sky_params.fog_sun_tint);
			lua_pop(L, 1);

			sky_params.fog_moon_tint = video::SColor(255, 255, 255, 255);
			lua_getfield(L, -1, "fog_moon_tint");
			read_color(L, -1, &sky_params.fog_moon_tint);
			lua_pop(L, 1);

			lua_getfield(L, -1, "fog_tint_type");
			if (!lua_isnil(L, -1))
				sky_params.fog_tint_type = luaL_checkstring(L, -1);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		lua_getfield(L, 2, "fog");
		if (lua_istable(L, -1)) {
			sky_params.fog_distance = getintfield_default(L, -1,
				"fog_distance", sky_params.fog_distance);
			sky_params.fog_start = getfloatfield_default(L, -1,
