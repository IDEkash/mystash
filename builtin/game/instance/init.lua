-- Roblox-Style Instance Engine for Luanti (Built-in)
-- Provides a modern, hierarchical data model (Workspace, Instances, Parents, Children, and custom CSS UI).

game = {
	Workspace = nil,
	Players = {},
}

local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM .. "instance" .. DIR_DELIM

-- Load core components
dofile(gamepath .. "base.lua")
dofile(gamepath .. "folder.lua")
dofile(gamepath .. "part.lua")
dofile(gamepath .. "camera.lua")
dofile(gamepath .. "bone.lua")
dofile(gamepath .. "gui.lua")
dofile(gamepath .. "tests.lua")

-- Global initialization
game.Workspace = Instance.new("Folder")
game.Workspace.Name = "Workspace"
game.workspace = game.Workspace

-- Automatic Player join & leave hooks to map players as hierarchical Workspace Instances
core.register_on_joinplayer(function(player)
	local name = player:get_player_name()

	-- 1. Create player session Folder container
	local player_folder = Instance.new("Folder")
	player_folder.Name = name
	player_folder.Parent = game.Workspace
	game.Players[name] = player_folder

	-- 2. Create hierarchical Workspace components
	local player_gui = Instance.new("Folder", player_folder)
	player_gui.Name = "PlayerGui"
	player_folder.PlayerGui = player_gui

	local camera = Instance.new("Camera", player_folder)
	camera.Name = "CurrentCamera"
	camera.PlayerRef = player

	local char = Instance.new("Part", player_folder)
	char.Name = "Character"
	char.ObjectRef = player

	-- 3. Initialize dynamic WebView rendering pipeline for this player
	game.ui_runtime.init_player(name)
end)

core.register_on_leaveplayer(function(player)
	local name = player:get_player_name()

	-- 1. Close active WebView display
	game.ui_runtime.close_player(name)

	-- 2. Recursively destroy player's Workspace tree (PlayerGui, Camera, Character)
	local folder = game.Players[name]
	if folder then
		folder:Destroy()
		game.Players[name] = nil
	end
end)
