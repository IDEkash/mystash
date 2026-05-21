
replacement_api = {}

function replacement_api.replace_parts(params)
	local target = params.target_entity
	local source_entity_name = params.source_entity
	local parts = params.parts

	if not target or not target:get_pos() then
		return false
	end

	local props = target:get_properties()
	props.visual_attachments = props.visual_attachments or {}

	-- We need source properties to get the mesh and textures
	-- This might be tricky if source_entity is just a name.
	-- For now, let's assume we can get it from core.registered_entities
	local source_def = core.registered_entities[source_entity_name]
	if not source_def then
		core.log("error", "replacement_api: Source entity " .. source_entity_name .. " not found")
		return false
	end

	for bone, source_bone in pairs(parts) do
		-- Remove existing replacement for this bone if it exists
		for i = #props.visual_attachments, 1, -1 do
			if props.visual_attachments[i].bone == bone then
				table.remove(props.visual_attachments, i)
			end
		end

		-- Hide the original bone
		target:set_bone_override(bone, { visible = false })

		local textures = source_def.initial_properties.textures
		local source_bone_name = source_bone
		local force_visible = false
		if type(source_bone) == "table" then
			if source_bone.textures then
				textures = source_bone.textures
			end
			if source_bone.force_visible ~= nil then
				force_visible = source_bone.force_visible
			end
			source_bone_name = source_bone.bone or source_bone[1]
		end

		-- Add visual attachment
		table.insert(props.visual_attachments, {
			mesh = source_def.initial_properties.mesh,
			bone = bone,
			source_bone = source_bone_name,
			position = {x=0, y=0, z=0},
			rotation = {x=0, y=0, z=0},
			scale = 1,
			textures = textures,
			inherit_animation = true,
			force_visible = force_visible,
		})
	end

	target:set_properties(props)
	return true
end

function replacement_api.restore_part(entity, bone)
	if not entity or not entity:get_pos() then return end

	local props = entity:get_properties()
	local found = false
	if props.visual_attachments then
		for i = #props.visual_attachments, 1, -1 do
			if props.visual_attachments[i].bone == bone then
				table.remove(props.visual_attachments, i)
				found = true
			end
		end
	end

	if found then
		entity:set_bone_override(bone, { visible = true })
		entity:set_properties(props)
	end
end

function replacement_api.clear_replacements(entity)
	if not entity or not entity:get_pos() then return end

	local props = entity:get_properties()
	if not props.visual_attachments then return end

	for _, att in ipairs(props.visual_attachments) do
		if att.bone then
			entity:set_bone_override(att.bone, { visible = true })
		end
	end

	props.visual_attachments = {}
	entity:set_properties(props)
end

function replacement_api.get_replaced_parts(entity)
	if not entity or not entity:get_pos() then return {} end
	local props = entity:get_properties()
	local res = {}
	if props.visual_attachments then
		for _, att in ipairs(props.visual_attachments) do
			if att.source_bone then
				table.insert(res, att.bone)
			end
		end
	end
	return res
end
