-- Example of RmlUi Integration in Luanti

local ui = core.rmlui.create("rmlui_example")

-- Load the RML document from the mod's ui/ folder
ui:load("mod://rmlui_example/ui/example.rml")
ui:show()

-- Listen to events on the document
ui:on("click", function(id)
	core.log("action", "[RmlUi Example] Clicked element ID: " .. tostring(id))
	if id == "close_btn" then
		core.log("action", "[RmlUi Example] Closing UI")
		ui:close()
	elseif id == "submit_btn" then
		local name_input = ui:find("name_input")
		if name_input then
			core.log("action", "[RmlUi Example] Submitted input.")
		end
	end
end)
