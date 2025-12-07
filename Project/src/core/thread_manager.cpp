#include "thread_manager.h"
#include <iostream>
#include <algorithm>

ThreadManager::ThreadManager(const ThreadPoolConfig &cfg) : config(cfg)
{
	// Initialize statistics
	stats.totalThreads = 0;
	stats.activeThreads = 0;
	stats.queuedTasks = 0;
	stats.completedTasks = 0;
	stats.failedTasks = 0;
	stats.averageTaskTime = std::chrono::milliseconds{0};
}

ThreadManager::~ThreadManager()
{
	shutdown();
}

bool ThreadManager::initialize(const ThreadPoolConfig &cfg)
{
	if (running.load())
	{
		std::cout << "[ThreadManager] Already initialized" << std::endl;
		return false;
	}

	config = cfg;
	running.store(true);
	shuttingDown.store(false);

	// Start minimum number of worker threads
	ensureMinimumThreads();
	return true;
}

void ThreadManager::shutdown()
{
	if (!running.load())
	{
		return;
	}

	std::cout << "[ThreadManager] Shutting down..." << std::endl;

	// Set shutdown flags
	running.store(false);
	shuttingDown.store(true);

	// Stop all named threads first
	stopAllNamedThreads();

	// Wake up all worker threads
	taskCondition.notify_all();

	// Wait for all worker threads to complete
	for (auto &worker : workers)
	{
		if (worker && worker->joinable())
		{
			worker->join();
		}
	}
	workers.clear();

	// Clear remaining tasks
	{
		std::lock_guard<std::mutex> lock(taskQueueMutex);
		while (!taskQueue.empty())
		{
			taskQueue.pop();
		}
	}

	// Reset statistics
	{
		std::lock_guard<std::mutex> lock(statsMutex);
		stats.totalThreads = 0;
		stats.activeThreads = 0;
		stats.queuedTasks = 0;
	}

	activeThreadCount.store(0);

	std::cout << "[ThreadManager] Shutdown complete" << std::endl;
}

void ThreadManager::submitTask(std::function<void()> task,
															 TaskPriority priority,
															 ThreadCategory category,
															 const std::string &name)
{
	if (shuttingDown.load())
	{
		std::cerr << "[ThreadManager] Cannot submit task during shutdown" << std::endl;
		return;
	}

	{
		std::lock_guard<std::mutex> lock(taskQueueMutex);

		// Check queue size limit
		if (taskQueue.size() >= config.maxQueueSize)
		{
			std::cerr << "[ThreadManager] Task queue full, dropping task: " << name << std::endl;
			{
				std::lock_guard<std::mutex> statsLock(statsMutex);
				stats.failedTasks++;
			}
			return;
		}

		taskQueue.emplace(std::move(task), priority, category, name);

		// Update statistics
		{
			std::lock_guard<std::mutex> statsLock(statsMutex);
			stats.queuedTasks = taskQueue.size();
			stats.tasksByCategory[category]++;
		}
	}

	// Ensure we have enough threads
	ensureMinimumThreads();

	// Wake up a worker thread
	taskCondition.notify_one();
}

bool ThreadManager::startNamedThread(const std::string &name,
																		 std::function<void()> threadFunction,
																		 ThreadCategory category)
{
	std::lock_guard<std::mutex> lock(namedThreadsMutex);

	if (namedThreads.find(name) != namedThreads.end())
	{
		std::cout << "[ThreadManager] Named thread '" << name << "' already exists" << std::endl;
		return false;
	}

	try
	{
		auto thread = std::make_unique<std::thread>([this, name, threadFunction, category]()
																								{
            std::cout << "[ThreadManager] Named thread '" << name << "' started" << std::endl;
            
            // Register thread category
            {
                std::lock_guard<std::mutex> mapLock(threadMapMutex);
                threadCategories[std::this_thread::get_id()] = category;
            }
            
            try {
                threadFunction();
            } catch (const std::exception& e) {
                std::cerr << "[ThreadManager] Named thread '" << name << "' error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ThreadManager] Named thread '" << name << "' unknown error" << std::endl;
            }
            
            // Unregister thread category
            {
                std::lock_guard<std::mutex> mapLock(threadMapMutex);
                threadCategories.erase(std::this_thread::get_id());
            }
            
            std::cout << "[ThreadManager] Named thread '" << name << "' ended" << std::endl; });

		namedThreads[name] = std::move(thread);

		std::cout << "[ThreadManager] Started named thread: " << name << std::endl;
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "[ThreadManager] Failed to start named thread '" << name << "': " << e.what() << std::endl;
		return false;
	}
}

void ThreadManager::stopNamedThread(const std::string &name)
{
	std::lock_guard<std::mutex> lock(namedThreadsMutex);

	auto it = namedThreads.find(name);
	if (it == namedThreads.end())
	{
		return;
	}

	if (it->second && it->second->joinable())
	{
		it->second->join();
	}

	namedThreads.erase(it);
	std::cout << "[ThreadManager] Stopped named thread: " << name << std::endl;
}

bool ThreadManager::isNamedThreadRunning(const std::string &name) const
{
	std::lock_guard<std::mutex> lock(namedThreadsMutex);

	auto it = namedThreads.find(name);
	return it != namedThreads.end() && it->second && it->second->joinable();
}

void ThreadManager::stopAllNamedThreads()
{
	std::lock_guard<std::mutex> lock(namedThreadsMutex);

	for (auto &[name, thread] : namedThreads)
	{
		if (thread && thread->joinable())
		{
			thread->join();
			std::cout << "[ThreadManager] Stopped named thread: " << name << std::endl;
		}
	}

	namedThreads.clear();
}

void ThreadManager::submitDataTask(std::function<void()> task, TaskPriority priority)
{
	submitTask(std::move(task), priority, ThreadCategory::DATA, "DataTask");
}

void ThreadManager::submitNetworkTask(std::function<void()> task, TaskPriority priority)
{
	submitTask(std::move(task), priority, ThreadCategory::NETWORK, "NetworkTask");
}

void ThreadManager::submitUITask(std::function<void()> task, TaskPriority priority)
{
	submitTask(std::move(task), priority, ThreadCategory::UI, "UITask");
}

void ThreadManager::submitBackgroundTask(std::function<void()> task, TaskPriority priority)
{
	submitTask(std::move(task), priority, ThreadCategory::BACKGROUND, "BackgroundTask");
}

void ThreadManager::workerLoop()
{
	while (running.load())
	{
		Task task([]() {}, TaskPriority::NORMAL, ThreadCategory::GENERAL, "");
		bool hasTask = false;

		{
			std::unique_lock<std::mutex> lock(taskQueueMutex);

			taskCondition.wait(lock, [this]()
												 { return !taskQueue.empty() || shuttingDown.load(); });

			if (shuttingDown.load())
			{
				break;
			}

			if (!taskQueue.empty())
			{
				task = std::move(const_cast<Task &>(taskQueue.top()));
				taskQueue.pop();
				hasTask = true;

				// Update statistics
				{
					std::lock_guard<std::mutex> statsLock(statsMutex);
					stats.queuedTasks = taskQueue.size();
				}
			}
		}

		if (hasTask)
		{
			activeThreadCount.fetch_add(1);

			auto startTime = std::chrono::steady_clock::now();
			bool success = true;

			try
			{
				task.function();
			}
			catch (const std::exception &e)
			{
				std::cerr << "[ThreadManager] Task execution error: " << e.what() << std::endl;
				success = false;
			}
			catch (...)
			{
				std::cerr << "[ThreadManager] Task execution unknown error" << std::endl;
				success = false;
			}

			auto endTime = std::chrono::steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

			updateStatistics(task, success, duration);
			activeThreadCount.fetch_sub(1);
		}
	}
}

void ThreadManager::updateStatistics(const Task &task, bool success, std::chrono::milliseconds duration)
{
	if (!config.enableStatistics)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(statsMutex);

	if (success)
	{
		stats.completedTasks++;
	}
	else
	{
		stats.failedTasks++;
	}

	// Update average task time
	size_t totalTasks = stats.completedTasks + stats.failedTasks;
	if (totalTasks > 0)
	{
		auto totalTime = stats.averageTaskTime.count() * (totalTasks - 1) + duration.count();
		stats.averageTaskTime = std::chrono::milliseconds(totalTime / totalTasks);
	}

	stats.activeThreads = activeThreadCount.load();
	stats.totalThreads = workers.size();
}

void ThreadManager::ensureMinimumThreads()
{
	std::lock_guard<std::mutex> lock(taskQueueMutex);

	while (workers.size() < config.minThreads && workers.size() < config.maxThreads)
	{
		try
		{
			workers.emplace_back(std::make_unique<std::thread>(&ThreadManager::workerLoop, this));
		}
		catch (const std::exception &e)
		{
			std::cerr << "[ThreadManager] Failed to create worker thread: " << e.what() << std::endl;
			break;
		}
	}
}

void ThreadManager::cleanupIdleThreads()
{
	// Implementation for cleanup idle threads could be added here
	// For now, we keep all worker threads alive
}

size_t ThreadManager::getCurrentThreadCount() const
{
	return workers.size();
}

size_t ThreadManager::getQueuedTaskCount() const
{
	std::lock_guard<std::mutex> lock(taskQueueMutex);
	return taskQueue.size();
}

ThreadPoolStats ThreadManager::getStatistics() const
{
	std::lock_guard<std::mutex> lock(statsMutex);
	return stats;
}

void ThreadManager::resetStatistics()
{
	std::lock_guard<std::mutex> lock(statsMutex);
	stats.completedTasks = 0;
	stats.failedTasks = 0;
	stats.averageTaskTime = std::chrono::milliseconds{0};
	stats.tasksByCategory.clear();
}

void ThreadManager::printStatus() const
{
	auto currentStats = getStatistics();

	std::cout << "\n=== ThreadManager Status ===" << std::endl;
	std::cout << "Total Threads: " << currentStats.totalThreads << std::endl;
	std::cout << "Active Threads: " << currentStats.activeThreads << std::endl;
	std::cout << "Queued Tasks: " << currentStats.queuedTasks << std::endl;
	std::cout << "Completed Tasks: " << currentStats.completedTasks << std::endl;
	std::cout << "Failed Tasks: " << currentStats.failedTasks << std::endl;
	std::cout << "Average Task Time: " << currentStats.averageTaskTime.count() << "ms" << std::endl;

	std::cout << "Tasks by Category:" << std::endl;
	for (const auto &[category, count] : currentStats.tasksByCategory)
	{
		std::string categoryName;
		switch (category)
		{
		case ThreadCategory::GENERAL:
			categoryName = "GENERAL";
			break;
		case ThreadCategory::DATA:
			categoryName = "DATA";
			break;
		case ThreadCategory::NETWORK:
			categoryName = "NETWORK";
			break;
		case ThreadCategory::UI:
			categoryName = "UI";
			break;
		case ThreadCategory::BACKGROUND:
			categoryName = "BACKGROUND";
			break;
		}
		std::cout << "  " << categoryName << ": " << count << std::endl;
	}
	std::cout << "=========================" << std::endl;
}

void ThreadManager::waitForAllTasks(std::chrono::milliseconds timeout)
{
	auto startTime = std::chrono::steady_clock::now();

	while (true)
	{
		size_t queuedTasks = getQueuedTaskCount();
		size_t activeTasks = getActiveThreadCount();

		if (queuedTasks == 0 && activeTasks == 0)
		{
			break;
		}

		if (timeout.count() > 0)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - startTime);
			if (elapsed >= timeout)
			{
				break;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void ThreadManager::waitForCategory(ThreadCategory category, std::chrono::milliseconds timeout)
{
	// Simple implementation - could be enhanced to track category-specific tasks
	waitForAllTasks(timeout);
}

void ThreadManager::setMaxThreads(size_t maxThreads)
{
	config.maxThreads = maxThreads;
}

void ThreadManager::setMinThreads(size_t minThreads)
{
	config.minThreads = minThreads;
	ensureMinimumThreads();
}

void ThreadManager::updateConfig(const ThreadPoolConfig &newConfig)
{
	config = newConfig;
	ensureMinimumThreads();
}