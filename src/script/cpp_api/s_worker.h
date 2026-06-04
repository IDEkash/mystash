// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "threading/thread.h"
#include "threading/semaphore.h"
#include "common/c_packer.h"
#include "cpp_api/s_base.h"

class WorkerEngine;
class Server;

struct WorkerJob {
	u32 worker_id;
	std::string serialized_func;
	std::unique_ptr<PackedValue> args;
	u32 job_id;
	std::string mod_origin;
	bool cancelled = false;
};

struct WorkerResult {
	u32 job_id;
	bool success;
	std::unique_ptr<PackedValue> result;
	std::string error;
	std::string mod_origin;
};

class LuaWorker : public std::enable_shared_from_this<LuaWorker> {
public:
	LuaWorker(WorkerEngine *engine, u32 id, const std::string &name);
	~LuaWorker();

	void runJob(WorkerJob &&job);
	u32 getId() const { return m_id; }
	const std::string &getName() const { return m_name; }
	u64 getLastActiveTime() const { return m_last_active_time; }

	bool isBusy() const { return m_busy; }
	void setBusy(bool busy) { m_busy = busy; }

private:
	static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize);
	static void l_hook(lua_State *L, lua_Debug *ar);

	WorkerEngine *m_engine;
	u32 m_id;
	std::string m_name;
	lua_State *L = nullptr;
	size_t m_memory_used = 0;
	u64 m_last_active_time;
	std::atomic<bool> m_timeout_tripped{false};
	u64 m_job_start_time = 0;
	std::atomic<bool> m_busy{false};

	friend class WorkerEngine;
};

class WorkerThread : public Thread {
public:
	WorkerThread(WorkerEngine *engine, const std::string &name);
	void *run() override;

private:
	WorkerEngine *m_engine;
};

class WorkerEngine {
public:
	WorkerEngine(Server *server);
	~WorkerEngine();

	void step(lua_State *L);

	u32 createWorker(const std::string &name);
	u32 queueJob(u32 worker_id, std::string &&func, PackedValue *args, const std::string &mod_origin);

private:
	void workerLoop();
	void putResult(WorkerResult &&result);
	void cleanupIdleWorkers();

	Server *m_server;
	u32 m_worker_id_counter = 0;
	u32 m_job_id_counter = 0;

	std::mutex m_workers_mutex;
	std::unordered_map<u32, std::shared_ptr<LuaWorker>> m_workers;

	std::mutex m_job_queue_mutex;
	std::deque<WorkerJob> m_job_queue;
	Semaphore m_job_semaphore;

	std::mutex m_result_queue_mutex;
	std::deque<WorkerResult> m_result_queue;

	std::vector<std::unique_ptr<WorkerThread>> m_threads;
	std::atomic<bool> m_running{true};

	u64 m_last_cleanup_time = 0;

	friend class WorkerThread;
	friend class LuaWorker;
};

class ScriptApiWorker : virtual public ScriptApiBase {
public:
	ScriptApiWorker(Server *server);
	~ScriptApiWorker() = default;

	void stepWorker();
	u32 createWorker(const std::string &name);
	u32 queueWorkerJob(u32 worker_id, std::string &&func, PackedValue *args, const std::string &mod_origin);

protected:
	WorkerEngine m_worker_engine;
};
