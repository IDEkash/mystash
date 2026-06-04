-- Worker system implementation

core.worker = core.worker or {}

local workers = {}
local callbacks = {}
local job_id_counter = 0

function core.worker.create(name)
	local id = core.worker._create(name)
	local worker = {
		id = id,
		name = name
	}
	workers[id] = worker
	return worker
end

function core.worker.run(worker, func, callback, ...)
	local serialized_func = string.dump(func)
	local args = {...}
	local mod_origin = core.get_current_modname() or ""

	local job_id = core.worker._queue(worker.id, serialized_func, args, mod_origin)
	callbacks[job_id] = callback
	return job_id
end

function core.worker.handle_results(job_id, success, result_or_error)
	local callback = callbacks[job_id]
	if not callback then
		return
	end
	callbacks[job_id] = nil

	if success then
		-- result_or_error is the result table (packed).
		-- If it was a table of results (packed), we unpack it.
		-- But our C++ code packs exactly one value from the stack.
		callback(true, result_or_error, nil)
	else
		callback(false, nil, result_or_error)
	end
end
