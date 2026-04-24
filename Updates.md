# Engine API Updates

This file documents new engine API functions exposed to modding systems.

## New Global Functions (Server-side)

### `core.get_forceloaded_blocks()`
Returns a list of forceloaded block positions.
* Returns: `{{x=num, y=num, z=num}, ...}`

### `core.get_active_block_count()`
Returns the number of currently active blocks in the environment.
* Returns: `number`

### `core.get_active_object_count()`
Returns the number of currently active objects (excluding players) in the environment.
* Returns: `number`

### `core.get_emerge_status()`
Returns information about the emerge manager.
* Returns: `{qlen=num}`
    * `qlen`: Current length of the emerge queue.

### `core.get_server_info()`
Returns general information about the server.
* Returns: `{uptime=num, status=string, proto_min=num, proto_max=num}`
    * `uptime`: Server uptime in seconds.
    * `status`: Server status string.
    * `proto_min`: Minimum supported protocol version.
    * `proto_max`: Maximum supported protocol version.

### `core.get_auth_database_info()`
Returns information about the authentication database.
* Returns: `{entry_count=num}`
    * `entry_count`: Number of accounts in the database.

### `core.get_player_database_info()`
Returns information about the player database/environment.
* Returns: `{online_count=num}`
    * `online_count`: Number of currently connected players.

### `core.get_mod_storage_info()`
Returns information about the mod storage database.
* Returns: `{mod_count=num}`
    * `mod_count`: Number of mods with data in the storage database.

## Updated Functions

### `core.get_game_info()`
Now includes an `engine` field with details about the running engine.
* Returns: `{..., engine={project=string, version=string, hash=string, build_info=string}}`

### `core.get_player_information(name)`
The following fields are now always available (previously only in debug builds):
* `serialization_version`: Engine serialization version.
* `major`: Engine major version.
* `minor`: Engine minor version.
* `patch`: Engine patch version.
* `state`: Player connection state.
