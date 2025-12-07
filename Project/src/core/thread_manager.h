#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <future>

// Task priority levels
enum class TaskPriority : int
{
	LOW = 0,
	NORMAL = 1,
	HIGH = 2,
	CRITICAL = 3
};

// Thread categories for different types of work
enum class ThreadCategory
{
	GENERAL,	 // General purpose tasks
	DATA,			 // Data processing
	NETWORK,	 // Network operations
	UI,				 // User interface tasks
	BACKGROUND // Background maintenance tasks
};

// Task wrapper with metadata
struct Task
{
	std::function<void()> function;
	TaskPriority priority;
	ThreadCategory category;
	std::string name;
	std::chrono::steady_clock::time_point createdAt;

	Task(std::function<void()> func, TaskPriority prio = TaskPriority::NORMAL,
			 ThreadCategory cat = ThreadCategory::GENERAL, const std::string &taskName = "")
			: function(std::move(func)), priority(prio), category(cat), name(taskName),
				createdAt(std::chrono::steady_clock::now()) {}

	// Comparison for priority queue (higher priority first)
	bool operator<(const Task &other) const
	{
		if (priority != other.priority)
		{
			return static_cast<int>(priority) < static_cast<int>(other.priority);
		}
		return createdAt > other.createdAt; // FIFO for same priority
	}
};

// Thread pool statistics
struct ThreadPoolStats
{
	size_t totalThreads = 0;
	size_t activeThreads = 0;
	size_t queuedTasks = 0;
	size_t completedTasks = 0;
	size_t failedTasks = 0;
	std::chrono::milliseconds averageTaskTime{0};
	std::map<ThreadCategory, size_t> tasksByCategory;
};

// Configuration for thread pool
struct ThreadPoolConfig
{
	size_t minThreads = 1;
	size_t maxThreads = std::thread::hardware_concurrency();
	size_t maxQueueSize = 1000;
	std::chrono::milliseconds threadIdleTimeout{30000}; // 30 seconds
	bool enableStatistics = true;

	ThreadPoolConfig()
	{
		if (maxThreads == 0)
			maxThreads = 4; // Fallback if hardware_concurrency() fails
	}
};

class ThreadManager
{
private:
	// Core thread pool management
	std::atomic<bool> running{false};
	std::atomic<bool> shuttingDown{false};
	std::vector<std::unique_ptr<std::thread>> workers;

	// Task queue with priority support
	mutable std::mutex taskQueueMutex;
	std::condition_variable taskCondition;
	std::priority_queue<Task> taskQueue;

	// Thread pool configuration and statistics
	ThreadPoolConfig config;
	ThreadPoolStats stats;
	mutable std::mutex statsMutex;

	// Active threads tracking
	std::atomic<size_t> activeThreadCount{0};
	std::map<std::thread::id, ThreadCategory> threadCategories;
	mutable std::mutex threadMapMutex;

	// Specialized thread management
	std::map<std::string, std::unique_ptr<std::thread>> namedThreads;
	mutable std::mutex namedThreadsMutex;

	// Internal worker functions
	void workerLoop();
	void updateStatistics(const Task &task, bool success, std::chrono::milliseconds duration);
	void cleanupIdleThreads();
	void ensureMinimumThreads();

public:
	explicit ThreadManager(const ThreadPoolConfig &cfg = ThreadPoolConfig{});
	~ThreadManager();

	// Core thread pool operations
	bool initialize(const ThreadPoolConfig &cfg = ThreadPoolConfig{});
	void shutdown();
	bool isInitialized() const { return running.load(); }

	// Task submission
	void submitTask(std::function<void()> task,
									TaskPriority priority = TaskPriority::NORMAL,
									ThreadCategory category = ThreadCategory::GENERAL,
									const std::string &name = "");

	template <typename F, typename... Args>
	auto submitTaskWithResult(F &&f, Args &&...args)
			-> std::future<typename std::result_of<F(Args...)>::type>
	{
		using ReturnType = typename std::result_of<F(Args...)>::type;

		auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::future<ReturnType> result = taskPtr->get_future();

		submitTask([taskPtr]()
							 { (*taskPtr)(); });

		return result;
	}

	// Specialized thread management
	bool startNamedThread(const std::string &name,
												std::function<void()> threadFunction,
												ThreadCategory category = ThreadCategory::GENERAL);
	void stopNamedThread(const std::string &name);
	bool isNamedThreadRunning(const std::string &name) const;
	void stopAllNamedThreads();

	// Convenience methods for common operations
	void submitDataTask(std::function<void()> task, TaskPriority priority = TaskPriority::NORMAL);
	void submitNetworkTask(std::function<void()> task, TaskPriority priority = TaskPriority::HIGH);
	void submitUITask(std::function<void()> task, TaskPriority priority = TaskPriority::HIGH);
	void submitBackgroundTask(std::function<void()> task, TaskPriority priority = TaskPriority::LOW);

	// Thread pool management
	void setMaxThreads(size_t maxThreads);
	void setMinThreads(size_t minThreads);
	size_t getCurrentThreadCount() const;
	size_t getActiveThreadCount() const { return activeThreadCount.load(); }
	size_t getQueuedTaskCount() const;

	// Statistics and monitoring
	ThreadPoolStats getStatistics() const;
	void resetStatistics();
	void printStatus() const;

	// Synchronization
	void waitForAllTasks(std::chrono::milliseconds timeout = std::chrono::milliseconds{0});
	void waitForCategory(ThreadCategory category, std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

	// Configuration
	const ThreadPoolConfig &getConfig() const { return config; }
	void updateConfig(const ThreadPoolConfig &newConfig);

	// Prevent copy/move
	ThreadManager(const ThreadManager &) = delete;
	ThreadManager &operator=(const ThreadManager &) = delete;
	ThreadManager(ThreadManager &&) = delete;
	ThreadManager &operator=(ThreadManager &&) = delete;
};