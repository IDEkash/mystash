-- Luanti
-- Copyright (C) 2014 sapier
-- SPDX-License-Identifier: LGPL-2.1-or-later

--------------------------------------------------------------------------------

local function rename_modpack_formspec(dialogdata)
	local retval =
		"formspec_version[10]size[11.5,4.5]" ..
		"style[te_modpack_name;bgcolor=#1e293b;textcolor=white;border=true;border_color=#334155]" ..
		"style[dlg_rename_modpack_confirm;bgcolor=#0284c7;textcolor=white;font=bold]" ..
		"style[dlg_rename_modpack_confirm:hovered;bgcolor=#0369a1]" ..
		"style[dlg_rename_modpack_cancel;bgcolor=#9b2c2c;textcolor=white]" ..
		"style[dlg_rename_modpack_cancel:hovered;bgcolor=#b91c1c]" ..

		-- Solid modern greyish-blue background (Layer 1)
		"box[-0.5,-0.5;12.5,5.5;#0f172aF2]" ..

		-- Rounded Container Card (Layer 2)
		"background9[0.15,0.15;11.2;4.2;button_hover_semitrans.png;false;6,6]" ..

		"button[2.5,3.3;3,0.8;dlg_rename_modpack_confirm;"..
				fgettext("Accept") .. "]" ..
		"button[6.0,3.3;3,0.8;dlg_rename_modpack_cancel;"..
				fgettext("Cancel") .. "]"

	local input_y = 1.8
	if dialogdata.mod.is_name_explicit then
		retval = retval .. "textarea[1,0.3;9.5,1.5;;;" ..
				fgettext("This modpack has an explicit name given in its modpack.conf " ..
						"which will override any renaming here.") .. "]"
		input_y = 2.1
	end
	retval = retval ..
		"field[2.25," .. input_y .. ";7,0.75;te_modpack_name;" ..
		fgettext("Rename Modpack:") .. ";" .. dialogdata.mod.dir_name .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function rename_modpack_buttonhandler(this, fields)
	if fields["dlg_rename_modpack_confirm"] ~= nil then
		local oldpath = this.data.mod.path
		local targetpath = this.data.mod.parent_dir .. DIR_DELIM .. fields["te_modpack_name"]
		os.rename(oldpath, targetpath)
		pkgmgr.reload_global_mods()
		pkgmgr.selected_mod = pkgmgr.global_mods:get_current_index(
			pkgmgr.global_mods:raw_index_by_uid(fields["te_modpack_name"]))

		this:delete()
		return true
	end

	if fields["dlg_rename_modpack_cancel"] then
		this:delete()
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function create_rename_modpack_dlg(modpack)

	local retval = dialog_create("dlg_delete_mod",
					rename_modpack_formspec,
					rename_modpack_buttonhandler,
					nil)
	retval.data.mod = modpack
	return retval
end
