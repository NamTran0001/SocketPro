#pragma once

#include <string>
#include <set>
#include <memory>
#include <atomic>
#include <functional>
#include <chrono>
#include "thread_manager.h"

// Forward declaration
class Logger;

class MailController
{
private:
	std::string username;
	std::string app_password;
	std::string imap_server;
	std::string smtp_server;
	std::set<std::string> processed_uids;
	std::string processed_file;
	std::string server_ip;
	int server_port;
	std::atomic<bool> is_running;
	std::atomic<bool> mail_command_enabled;
	// Use ThreadManager reference instead of creating own
	ThreadManager &thread_manager;
	std::unique_ptr<Logger> logger;
	std::chrono::system_clock::time_point startup_time;

	std::function<std::string(const std::string &)> command_callback;

	std::string exec(const char *cmd);
	std::string sendCommandToServer(const std::string &command);
	void loadProcessedUIDs();
	void saveProcessedUID(const std::string &uid);
	bool isValidCommand(const std::string &subject, std::string &command);
	std::string processCommand(const std::string &command);
	std::string getLocalSystemStatus();
	std::string getAvailableCommands();
	bool sendReport(const std::string &to_email, const std::string &command, const std::string &report);
	std::string getCurrentTime();
	std::chrono::system_clock::time_point parseEmailDate(const std::string &dateStr);
	bool isEmailAfterStartup(const std::string &emailContent);
	void processEmail(const std::string &uid);
	void emailMonitoringLoop(int interval_seconds);

public:
	MailController(const std::string &user, const std::string &pass,
								 const std::string &srv_ip, int srv_port, ThreadManager &threadMgr);
	~MailController();

	void setCommandCallback(std::function<std::string(const std::string &)> callback);

	void checkNewEmails();
	void startBackgroundMonitoring(int interval_seconds = 10);
	void stopMonitoring();
	bool isMonitoring() const { return is_running.load(); }

	void enableMailCommand(bool enable = true);
	void disableMailCommand();
	bool isMailCommandEnabled() const { return mail_command_enabled.load(); }

	std::string getStatus() const;
};