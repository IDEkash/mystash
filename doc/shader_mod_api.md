# Mod Shader API

Luanti allows mods to provide custom GLSL shaders to override existing engine shaders.
This is done through the Lua API and a specific folder convention.

## Folder Convention

Mods can include a `shaders/` directory. All files in this directory are automatically sent to the client.

Example:
```
my_shader_mod/
  mod.conf          ← add: shader_api = 1
  init.lua          ← calls core.register_shader()
  shaders/
    nodes_shader_fragment.glsl
    nodes_shader_vertex.glsl
```

## Lua API

### `core.register_shader(def)`

Registers a shader override.

`def` is a table containing:
* `name`: A unique name for this mod shader (e.g., `"my_mod.cool_nodes"`).
* `target`: The engine shader to override.
* `stage`: The shader stage to override. One of `"vertex"`, `"fragment"`, or `"both"`.
* `path`: Absolute path to the GLSL file.
* `priority`: Numeric priority. If multiple mods override the same target, the one with the highest priority wins.

Example:
```lua
core.register_shader({
    name = "cool_shaders.nodes",
    target = "nodes_shader",
    stage = "fragment",
    path = core.get_modpath("cool_shaders") .. "/shaders/nodes_frag.glsl",
    priority = 10,
})
```

### `core.set_shader_uniform(shader_name, uniform_name, value)`

Sets a uniform value for a registered mod shader.

* `shader_name`: The name used when registering the shader.
* `uniform_name`: The name of the uniform in the GLSL code.
* `value`: The value to set. Can be a number (float/int), a boolean (converted to 0 or 1), or a table `{x=..., y=...}` for `vec2` or `{x=..., y=..., z=...}` for `vec3`.

Example:
```lua
core.set_shader_uniform("cool_shaders.nodes", "u_time", os.clock())
```

### `core.get_shader_names()`

Returns a list of all overridable engine shader targets.

Currently overridable targets:
* `nodes_shader`
* `object_shader`
* `cloud_shader`
* `shadow`
* `second_stage`
* `bloom_downsample`
* `bloom_upsample`
* `blur_h`
* `blur_v`
* `fxaa`
* `stars_shader`
* `minimap_shader`
* `extract_bloom`
* `update_exposure`

## Overriding logic

If a mod shader fails to compile, Luanti will log an error and fallback to the engine default shader.
Mod shaders should try to be compatible with engine-provided uniforms and attributes.
Common Luanti uniforms like `mWorldViewProj`, `mWorld`, `mTexture`, and `baseTexture` are available.
Mod-specific uniforms can be injected using `core.set_shader_uniform`.
