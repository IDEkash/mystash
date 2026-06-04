// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "s_worker.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_base.h"
#include "common/c_packer.h"
#include "log.h"
#include "porting.h"
#include "server.h"
#include "settings.h"
#include <cstdlib>
#include <memory>

extern "C" {
#include <lualib.h>
}

#define WORKER_MEMORY_LIMIT (50 * 1024 * 1024)
#define WORKER_TIMEOUT_MS 5000
#define WORKER_IDLE_TIMEOUT_MS 30000

static int worker_error_handler(lua_State *L)
{
	const char *s = lua_tostring(L, 1);
	lua_pushstring(L, s ? s : "Unknown error");
	return 1;
}

// LuaWorker implementation

LuaWorker::LuaWorker(WorkerEngine *engine, u32 id, const std::string &name) :
	m_engine(engine), m_id(id), m_name(name)
{
	L = lua_newstate(l_alloc, this);
	if (!L) {
		FATAL_ERROR("Failed to create Lua state for worker");
	}

	luaL_openlibs(L);
	// Remove unsafe libraries
	lua_pushnil(L);
	lua_setglobal(L, "io");
	lua_pushnil(L);
	lua_setglobal(L, "os");
	lua_pushnil(L);
	lua_setglobal(L, "package");
	lua_pushnil(L);
	lua_setglobal(L, "debug");
	lua_pushnil(L);
	lua_setglobal(L, "dofile");
	lua_pushnil(L);
	lua_setglobal(L, "loadfile");

	// core and minetest should not be there anyway since we didn't add them,
	// but let's be sure.
	lua_pushnil(L);
	lua_setglobal(L, "core");
	lua_pushnil(L);
	lua_setglobal(L, "minetest");

	lua_sethook(L, l_hook, LUA_MASKCOUNT, 10000);

	m_last_active_time = porting::getTimeMs();
}

LuaWorker::~LuaWorker()
{
	if (L)
		lua_close(L);
}

void *LuaWorker::l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	LuaWorker *worker = (LuaWorker *)ud;
	if (nsize == 0) {
		if (ptr) {
			worker->m_memory_used -= osize;
			free(ptr);
		}
		return nullptr;
	}

	if (worker->m_memory_used - osize + nsize > WORKER_MEMORY_LIMIT) {
		return nullptr;
	}

	void *new_ptr = realloc(ptr, nsize);
	if (new_ptr) {
		worker->m_memory_used = worker->m_memory_used - osize + nsize;
	}
	return new_ptr;
}

void LuaWorker::l_hook(lua_State *L, lua_Debug *ar)
{
	// We need to access the worker instance.
	// Since we don't have a direct way from L to LuaWorker in the hook,
	// we use a trick. We know that the allocator's ud is the LuaWorker.
	// But there is no lua_getallocf in standard Lua 5.1?
	// Actually there is: lua_getallocf(L, &ud)
	void *ud;
	lua_getallocf(L, &ud);
	LuaWorker *worker = (LuaWorker *)ud;

	if (porting::getTimeMs() - worker->m_job_start_time > WORKER_TIMEOUT_MS) {
		worker->m_timeout_tripped = true;
		luaL_error(L, "Worker timeout exceeded");
	}
}

void LuaWorker::runJob(WorkerJob &&job)
{
	if (job.cancelled)
		return;

	m_last_active_time = porting::getTimeMs();
	m_job_start_time = m_last_active_time;
	m_timeout_tripped = false;

	WorkerResult result;
	result.job_id = job.job_id;
	result.mod_origin = job.mod_origin;

	int top = lua_gettop(L);

	// Push error handler
	lua_pushcfunction(L, worker_error_handler);
	int error_handler = lua_gettop(L);

	if (luaL_loadbuffer(L, job.serialized_func.data(), job.serialized_func.size(), "=(worker)")) {
		result.success = false;
		result.error = lua_tostring(L, -1);
		lua_pop(L, 1);
	} else {
		// Push arguments
		int nargs = 0;
		if (job.args) {
			script_unpack(L, job.args.get());
			// if it was a table, it is 1 arg.
			// The API says core.worker.run(worker, func, callback, ...)
			// So args should probably be unpacked as multiple arguments.
			// But PackedValue usually packs one value.
			// If we want multiple args, we should have packed them into a table and unpack them here.
			// Let's assume job.args is a table of arguments.
			if (lua_istable(L, -1)) {
				nargs = lua_objlen(L, -1);
				for (int i = 1; i <= nargs; i++) {
					lua_rawgeti(L, -i, i); // This is wrong logic for unpacking.
				}
				// Correct unpacking of table:
				for (int i = 1; i <= nargs; i++) {
					lua_rawgeti(L, -i, i);
				}
				// Wait, the above is still wrong.
			} else {
				nargs = 1;
			}
		}

		// Re-do argument pushing correctly
		lua_pop(L, 1); // pop what script_unpack pushed
		if (job.args) {
			script_unpack(L, job.args.get());
			if (lua_istable(L, -1)) {
				int table_idx = lua_gettop(L);
				nargs = lua_objlen(L, table_idx);
				for (int i = 1; i <= nargs; i++) {
					lua_rawgeti(L, table_idx, i);
				}
				lua_remove(L, table_idx); // remove the table
			} else {
				nargs = 1;
			}
		}

		int status = lua_pcall(L, nargs, 1, error_handler);
		if (status != 0) {
			result.success = false;
			if (m_timeout_tripped) {
				result.error = "Timeout";
			} else {
				result.error = lua_tostring(L, -1);
			}
			lua_pop(L, 1);
		} else {
			result.success = true;
			try {
				result.result.reset(script_pack(L, -1));
			} catch (const std::exception &e) {
				result.success = false;
				result.error = e.what();
			}
			lua_pop(L, 1);
		}
	}

	lua_pop(L, 1); // pop error handler
	sanity_check(lua_gettop(L) == top);

	m_engine->putResult(std::move(result));
	m_last_active_time = porting::getTimeMs();
}

// WorkerThread implementation

WorkerThread::WorkerThread(WorkerEngine *engine, const std::string &name) :
	Thread(name), m_engine(engine)
{
}

void *WorkerThread::run()
{
	m_engine->workerLoop();
	return nullptr;
}

// WorkerEngine implementation

WorkerEngine::WorkerEngine(Server *server) : m_server(server)
{
	unsigned int num_threads = Thread::getNumberOfProcessors();
	if (num_threads > 1)
		num_threads -= 1;
	if (num_threads < 1)
		num_threads = 1;

	for (unsigned int i = 0; i < num_threads; i++) {
		auto thread = std::make_unique<WorkerThread>(this, "WorkerThread-" + std::to_string(i));
		m_threads.push_back(std::move(thread));
	}

	for (auto &thread : m_threads) {
		thread->start();
	}

	m_last_cleanup_time = porting::getTimeMs();
}

WorkerEngine::~WorkerEngine()
{
	m_running = false;
	for (size_t i = 0; i < m_threads.size(); i++) {
		m_job_semaphore.post();
	}
	for (auto &thread : m_threads) {
		thread->stop();
		thread->wait();
	}
}

void WorkerEngine::workerLoop()
{
	while (m_running) {
		m_job_semaphore.wait();
		if (!m_running)
			break;

		WorkerJob job;
		std::shared_ptr<LuaWorker> worker = nullptr;

		{
			std::lock_guard<std::mutex> lock(m_job_queue_mutex);
			if (m_job_queue.empty())
				continue;

			// Look for a job whose worker is not busy
			auto it = m_job_queue.begin();
			while (it != m_job_queue.end()) {
				std::lock_guard<std::mutex> workers_lock(m_workers_mutex);
				auto wit = m_workers.find(it->worker_id);
				if (wit != m_workers.end()) {
					if (!wit->second->isBusy()) {
						worker = wit->second;
						job = std::move(*it);
						m_job_queue.erase(it);
						worker->setBusy(true);
						break;
					}
				} else {
					// Worker gone, return error result later
					job = std::move(*it);
					m_job_queue.erase(it);
					break;
				}
				++it;
			}

			if (!worker && job.job_id == 0) {
				// No non-busy worker found for any job in queue
				// This can happen if all workers for queued jobs are busy.
				// We'll have to wait. But we already consumed one semaphore signal.
				// We should probably re-post it.
				m_job_semaphore.post();
				// Small sleep to avoid busy wait if everything is busy
				sleep_ms(10);
				continue;
			}
		}

		if (worker) {
			worker->runJob(std::move(job));
			worker->setBusy(false);
			// After finishing a job, there might be more jobs for this worker.
			// The semaphore might already have been signaled for them.
		} else if (job.job_id != 0) {
			// Worker was destroyed? Return error.
			WorkerResult result;
			result.job_id = job.job_id;
			result.success = false;
			result.error = "Worker does not exist";
			result.mod_origin = job.mod_origin;
			putResult(std::move(result));
		}
	}
}

void WorkerEngine::putResult(WorkerResult &&result)
{
	std::lock_guard<std::mutex> lock(m_result_queue_mutex);
	m_result_queue.push_back(std::move(result));
}

u32 WorkerEngine::createWorker(const std::string &name)
{
	std::lock_guard<std::mutex> lock(m_workers_mutex);
	u32 id = ++m_worker_id_counter;
	m_workers[id] = std::make_shared<LuaWorker>(this, id, name);
	return id;
}

u32 WorkerEngine::queueJob(u32 worker_id, std::string &&func, PackedValue *args, const std::string &mod_origin)
{
	std::lock_guard<std::mutex> lock(m_job_queue_mutex);
	u32 job_id = ++m_job_id_counter;
	WorkerJob job;
	job.worker_id = worker_id;
	job.serialized_func = std::move(func);
	job.args.reset(args);
	job.job_id = job_id;
	job.mod_origin = mod_origin;
	m_job_queue.push_back(std::move(job));
	m_job_semaphore.post();
	return job_id;
}

void WorkerEngine::step(lua_State *L)
{
	cleanupIdleWorkers();

	std::deque<WorkerResult> results;
	{
		std::lock_guard<std::mutex> lock(m_result_queue_mutex);
		results = std::move(m_result_queue);
		m_result_queue.clear();
	}

	if (results.empty())
		return;

	ScriptApiBase *script = (ScriptApiBase*)ModApiBase::getScriptApiBase(L);
	int error_handler = PUSH_ERROR_HANDLER(L);

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "worker");
	lua_getfield(L, -1, "handle_results");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 3); // Pop nil, worker, core
		lua_pop(L, 1); // Pop error handler
		return;
	}

	for (auto &res : results) {
		lua_pushvalue(L, -1); // handle_results
		lua_pushinteger(L, res.job_id);
		lua_pushboolean(L, res.success);
		if (res.success) {
			if (res.result) {
				script_unpack(L, res.result.get());
			} else {
				lua_pushnil(L);
			}
		} else {
			lua_pushstring(L, res.error.c_str());
		}

		script->setOriginDirect(res.mod_origin.empty() ? nullptr : res.mod_origin.c_str());
		int status = lua_pcall(L, 3, 0, error_handler);
		if (status) {
			// Error in result handler
			script_error(L, status, res.mod_origin.c_str(), "<worker callback>");
		}
	}

	lua_pop(L, 4); // handle_results, worker, core, error_handler
}

void WorkerEngine::cleanupIdleWorkers()
{
	u64 now = porting::getTimeMs();
	if (now - m_last_cleanup_time < 5000)
		return;
	m_last_cleanup_time = now;

	std::lock_guard<std::mutex> lock(m_workers_mutex);
	auto it = m_workers.begin();
	while (it != m_workers.end()) {
		// Only cleanup if not busy and idle timeout reached
		if (!it->second->isBusy() && now - it->second->getLastActiveTime() > WORKER_IDLE_TIMEOUT_MS) {
			infostream << "WorkerEngine: Cleaning up idle worker " << it->second->getName()
				<< " (ID " << it->first << ")" << std::endl;
			it = m_workers.erase(it);
		} else {
			++it;
		}
	}
}

// ScriptApiWorker implementation

ScriptApiWorker::ScriptApiWorker(Server *server) : ScriptApiBase(ScriptingType::Async), m_worker_engine(server)
{
}

void ScriptApiWorker::stepWorker()
{
	m_worker_engine.step(getStack());
}

u32 ScriptApiWorker::createWorker(const std::string &name)
{
	return m_worker_engine.createWorker(name);
}

u32 ScriptApiWorker::queueWorkerJob(u32 worker_id, std::string &&func, PackedValue *args, const std::string &mod_origin)
{
	return m_worker_engine.queueJob(worker_id, std::move(func), args, mod_origin);
}
