-- Luanti
-- Copyright (C) 2014 sapier
-- SPDX-License-Identifier: LGPL-2.1-or-later


--------------------------------------------------------------------------------
-- A tabview implementation                                                   --
-- Usage:                                                                     --
-- tabview.create: returns initialized tabview raw element                    --
-- element.add(tab): add a tab declaration                                    --
-- element.handle_buttons()                                                   --
-- element.handle_events()                                                    --
-- element.getFormspec() returns formspec of tabview                          --
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
local function add_tab(self,tab)
	assert(tab.size == nil or (type(tab.size) == table and
			tab.size.x ~= nil and tab.size.y ~= nil))
	assert(tab.cbf_formspec ~= nil and type(tab.cbf_formspec) == "function")
	assert(tab.cbf_button_handler == nil or
			type(tab.cbf_button_handler) == "function")
	assert(tab.cbf_events == nil or type(tab.cbf_events) == "function")

	local newtab = {
		name = tab.name,
		caption = tab.caption,
		icon = tab.icon,
		button_handler = tab.cbf_button_handler,
		event_handler = tab.cbf_events,
		get_formspec = tab.cbf_formspec,
		tabsize = tab.tabsize,
		formspec_version = tab.formspec_version or 6,
		on_change = tab.on_change,
		tabdata = {},
	}

	self.tablist[#self.tablist + 1] = newtab

	if self.last_tab_index == #self.tablist then
		self.current_tab = tab.name
		if tab.on_activate ~= nil then
			tab.on_activate(nil,tab.name)
		end
	end
end

--------------------------------------------------------------------------------
local function get_formspec(self)
	if self.hidden or (self.parent ~= nil and self.parent.hidden) then
		return ""
	end
	local tab = self.tablist[self.last_tab_index]

	local content, prepend = tab.get_formspec(self, tab.name, tab.tabdata, tab.tabsize)

	local TOUCH_GUI = core.settings:get_bool("touch_gui")

	local orig_tsize = tab.tabsize or { width = self.width, height = self.height }
	local tsize = { width = orig_tsize.width, height = orig_tsize.height }
	tsize.height = tsize.height
		+ TABHEADER_H -- tabheader included in formspec size
		+ (TOUCH_GUI and GAMEBAR_OFFSET_TOUCH or GAMEBAR_OFFSET_DESKTOP)
		+ GAMEBAR_H -- gamebar included in formspec size

	if self.parent == nil and not prepend then
		prepend = string.format("size[%f,%f,%s]", tsize.width, tsize.height,
				dump(self.fixed_size))

		local anchor_pos = TABHEADER_H + orig_tsize.height / 2
		prepend = prepend .. ("anchor[0.5,%f]"):format(anchor_pos / tsize.height)

		if tab.formspec_version then
			prepend = ("formspec_version[%d]"):format(tab.formspec_version) .. prepend
		end
	end

	local end_button_size = 0.75

	local tab_header_size = { width = tsize.width, height = TABHEADER_H }
	if self.end_button then
		tab_header_size.width = tab_header_size.width - end_button_size - 0.1
	end

	local formspec = (prepend or "") .. (self.prepend or "")

	if self.sidebar then
		local sidebar_w = self.sidebar_w or 4
		formspec = formspec .. ("bgcolor[;neither]container[0,0]box[0,0;%f,%f;#050505ee]"):format(
				sidebar_w, tsize.height)

		-- Logo/Header Area
		formspec = formspec .. ("image[0.8,0.4;%f,1.2;logo.png]"):format(sidebar_w - 1.6)

		formspec = formspec .. self:tab_header({width = sidebar_w, height = TABHEADER_H})

		formspec = formspec .. ("container[%f,0]box[-0.05,0;0.05,%f;#ffffff10]box[0,0;%f,%f;#00000060]"):format(
				sidebar_w, tsize.height, orig_tsize.width - sidebar_w, tsize.height)
		formspec = formspec .. content .. "container_end[]"

		if self.end_button then
			formspec = formspec ..
					("style[%s;noclip=true;border=false;bgcolor=#ffffff08;bgcolor_hover=#ffffff18;textcolor=#ffffff;content_offset=0]"):format(self.end_button.name) ..
					("tooltip[%s;%s]"):format(self.end_button.name, self.end_button.label) ..
					("image_button[0.2,%f;%f,1.2;%s;%s;%s]"):format(
							tsize.height - 1.6,
							sidebar_w - 0.4,
							core.formspec_escape(self.end_button.icon),
							self.end_button.name,
							core.formspec_escape(self.end_button.label))
		end

		formspec = formspec .. "container_end[]"
	else
		formspec = formspec .. ("bgcolor[;neither]container[0,%f]box[0,0;%f,%f;#0000008C]"):format(
				TABHEADER_H, orig_tsize.width, orig_tsize.height)
		formspec = formspec .. self:tab_header(tab_header_size) .. content

		if self.end_button then
			formspec = formspec ..
					("style[%s;noclip=true;border=false]"):format(self.end_button.name) ..
					("tooltip[%s;%s]"):format(self.end_button.name, self.end_button.label) ..
					("image_button[%f,%f;%f,%f;%s;%s;]"):format(
							self.width - end_button_size,
							(-tab_header_size.height - end_button_size) / 2,
							end_button_size,
							end_button_size,
							core.formspec_escape(self.end_button.icon),
							self.end_button.name)
		end

		formspec = formspec .. "container_end[]"
	end

	return formspec
end

--------------------------------------------------------------------------------
local function handle_buttons(self,fields)

	if self.hidden then
		return false
	end

	if self:handle_tab_buttons(fields) then
		return true
	end

	if self.end_button and fields[self.end_button.name] then
		return self.end_button.on_click(self)
	end

	if self.glb_btn_handler ~= nil and
		self.glb_btn_handler(self, fields) then
		return true
	end

	local tab = self.tablist[self.last_tab_index]
	if tab.button_handler ~= nil then
		return tab.button_handler(self, fields, tab.name, tab.tabdata)
	end

	return false
end

--------------------------------------------------------------------------------
local function handle_events(self,event)

	if self.hidden then
		return false
	end

	if self.glb_evt_handler ~= nil and
		self.glb_evt_handler(self,event) then
		return true
	end

	local tab = self.tablist[self.last_tab_index]
	if tab.evt_handler ~= nil then
		return tab.evt_handler(self, event, tab.name, tab.tabdata)
	end

	return false
end


--------------------------------------------------------------------------------
local function tab_header(self, size)
	local toadd = "container[" .. self.header_x .. "," .. self.header_y .. "]"

	if self.sidebar then
		local y = 2.0
		local sidebar_w = size.width
		for i = 1, #self.tablist do
			local tab = self.tablist[i]
			local caption = tab.caption
			if type(caption) == "function" then
				caption = caption(self)
			end

			local btn_name = self.name .. "_tab_" .. i
			if i == self.last_tab_index then
				toadd = toadd .. ("style[%s;bgcolor=#ffffff15;font=bold;textcolor=#44ccff;content_offset=0]"):format(btn_name)
				toadd = toadd .. ("box[0.1,%f;0.1,1.2;#44ccff]"):format(y)
			else
				toadd = toadd .. ("style[%s;bgcolor=#00000000;textcolor=#ffffff;content_offset=0]"):format(btn_name)
			end

			if tab.icon then
				toadd = toadd .. ("image_button[0.2,%f;%f,1.2;%s;%s;%s]"):format(
					y, sidebar_w - 0.4, core.formspec_escape(tab.icon), btn_name, core.formspec_escape(caption))
			else
				toadd = toadd .. ("button[0.2,%f;%f,1.2;%s;%s]"):format(
					y, sidebar_w - 0.4, btn_name, core.formspec_escape(caption))
			end
			y = y + 1.3
		end
	else
		local x = 0
		local tab_w = size.width / #self.tablist

		for i = 1, #self.tablist do
			local caption = self.tablist[i].caption
			if type(caption) == "function" then
				caption = caption(self)
			end

			local btn_name = self.name .. "_tab_" .. i
			if i == self.last_tab_index then
				toadd = toadd .. ("style[%s;bgcolor=#ffffff40;font=bold]"):format(btn_name)
			else
				toadd = toadd .. ("style[%s;bgcolor=#ffffff10]"):format(btn_name)
			end

			toadd = toadd .. ("button[%f,0;%f,%f;%s;%s]"):format(
				x, tab_w, size.height, btn_name, core.formspec_escape(caption))
			x = x + tab_w
		end
	end
	toadd = toadd .. "container_end[]"
	return toadd
end

--------------------------------------------------------------------------------
local function switch_to_tab(self, index)
	--first call on_change for tab to leave
	if self.tablist[self.last_tab_index].on_change ~= nil then
		self.tablist[self.last_tab_index].on_change("LEAVE",
				self.current_tab, self.tablist[index].name)
	end

	--update tabview data
	self.last_tab_index = index
	local old_tab = self.current_tab
	self.current_tab = self.tablist[index].name

	if (self.autosave_tab) then
		core.settings:set(self.name .. "_LAST",self.current_tab)
	end

	-- call for tab to enter
	if self.tablist[index].on_change ~= nil then
		self.tablist[index].on_change("ENTER",
				old_tab,self.current_tab)
	end
end

--------------------------------------------------------------------------------
local function handle_tab_buttons(self,fields)
	--save tab selection to config file
	if fields[self.name] then
		local index = tonumber(fields[self.name])
		switch_to_tab(self, index)
		return true
	end

	for i = 1, #self.tablist do
		if fields[self.name .. "_tab_" .. i] then
			switch_to_tab(self, i)
			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
local function set_tab_by_name(self, name)
	for i=1,#self.tablist,1 do
		if self.tablist[i].name == name then
			switch_to_tab(self, i)
			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
local function hide_tabview(self)
	self.hidden=true

	--call on_change as we're not gonna show self tab any longer
	if self.tablist[self.last_tab_index].on_change ~= nil then
		self.tablist[self.last_tab_index].on_change("LEAVE",
				self.current_tab, nil)
	end
end

--------------------------------------------------------------------------------
local function show_tabview(self)
	self.hidden=false

	-- call for tab to enter
	if self.tablist[self.last_tab_index].on_change ~= nil then
		self.tablist[self.last_tab_index].on_change("ENTER",
				nil,self.current_tab)
	end
end

local tabview_metatable = {
	add                       = add_tab,
	handle_buttons            = handle_buttons,
	handle_events             = handle_events,
	get_formspec              = get_formspec,
	show                      = show_tabview,
	hide                      = hide_tabview,
	delete                    = function(self) ui.delete(self) end,
	set_parent                = function(self,parent) self.parent = parent end,
	set_autosave_tab          =
			function(self,value) self.autosave_tab = value end,
	set_tab                   = set_tab_by_name,
	set_global_button_handler =
			function(self,handler) self.glb_btn_handler = handler end,
	set_global_event_handler =
			function(self,handler) self.glb_evt_handler = handler end,
	set_fixed_size =
			function(self,state) self.fixed_size = state end,
	set_end_button =
			function(self, v) self.end_button = v end,
	tab_header = tab_header,
	handle_tab_buttons = handle_tab_buttons
}

tabview_metatable.__index = tabview_metatable

--------------------------------------------------------------------------------
function tabview_create(name, size, tabheaderpos)
	local self = {}

	self.name     = name
	self.type     = "toplevel"
	self.width    = size.x
	self.height   = size.y
	self.header_x = tabheaderpos.x
	self.header_y = tabheaderpos.y

	setmetatable(self, tabview_metatable)

	self.fixed_size     = true
	self.hidden         = true
	self.current_tab    = nil
	self.last_tab_index = 1
	self.tablist        = {}

	self.autosave_tab   = false

	ui.add(self)
	return self
end
