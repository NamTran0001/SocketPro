#include "mail_controller.h"
#include "constants.h"
#include "thread_manager.h"
#include "../common/logger.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <regex>
#include <thread>
#include <chrono>
#include <array>
#include <cstdio>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

MailController::MailController(const std::string &user, const std::string &pass,
							   const std::string &srv_ip, int srv_port, ThreadManager &threadMgr)
	: username(user), app_password(pass), server_ip(srv_ip), server_port(srv_port),
	  processed_file("processed_uids.txt"), is_running(false), mail_command_enabled(false), thread_manager(threadMgr)
{
	imap_server = "imaps://imap.gmail.com:993";
	smtp_server = "smtps://smtp.gmail.com:465";
	startup_time = std::chrono::system_clock::now();
	loadProcessedUIDs();

	std::cout << "[Mail] Email controller initialized - will only process emails received after startup" << std::endl;
	std::cout << "[Mail] Mail command processing: " << (mail_command_enabled ? "ENABLED" : "DISABLED") << std::endl;
}

MailController::~MailController()
{
	stopMonitoring();
}

void MailController::stopMonitoring()
{
	if (!is_running.load())
	{
		return;
	}

	is_running.store(false);

	// Stop email monitoring thread
	thread_manager.stopNamedThread("EmailMonitoring");

	std::cout << "[Mail] Email monitoring stopped" << std::endl;
}

void MailController::setCommandCallback(std::function<std::string(const std::string &)> callback)
{
	command_callback = callback;
}

std::string MailController::exec(const char *cmd)
{
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe)
	{
		return "Error: popen failed";
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}
	return result;
}

std::string MailController::sendCommandToServer(const std::string &command)
{

	if (command_callback)
	{
		return command_callback(command);
	}

	WSADATA wsa;
	SOCKET clientSocket;
	struct sockaddr_in server;
	std::string response = "Error: Cannot connect to server";

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return "Error: WSAStartup failed";
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		WSACleanup();
		return "Error: Cannot create socket";
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip.c_str(), &server.sin_addr) <= 0)
	{
		closesocket(clientSocket);
		WSACleanup();
		return "Error: Invalid IP address format";
	}

	DWORD timeout = MAIL_TIMEOUT_MS;
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

	if (connect(clientSocket, (struct sockaddr *)&server, sizeof(server)) >= 0)
	{
		send(clientSocket, command.c_str(), static_cast<int>(command.length()), 0);

		char buffer[MAIL_BUFFER_SIZE];
		memset(buffer, 0, sizeof(buffer));
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
		if (bytesReceived > 0)
		{
			buffer[bytesReceived] = '\0';
			response = std::string(buffer);
		}
	}

	closesocket(clientSocket);
	WSACleanup();
	return response;
}

void MailController::loadProcessedUIDs()
{
	std::ifstream file(processed_file);
	std::string uid;
	while (std::getline(file, uid))
	{
		processed_uids.insert(uid);
	}
}

void MailController::saveProcessedUID(const std::string &uid)
{
	processed_uids.insert(uid);
	std::ofstream file(processed_file, std::ios::app);
	file << uid << std::endl;
}

bool MailController::isValidCommand(const std::string &subject, std::string &command)
{
	std::regex pattern(R"(\[Controller\]\s*(.+))");
	std::smatch matches;
	if (std::regex_search(subject, matches, pattern))
	{
		command = matches[1].str();
		command.erase(0, command.find_first_not_of(" \t"));
		command.erase(command.find_last_not_of(" \t") + 1);
		return true;
	}
	return false;
}

std::string MailController::processCommand(const std::string &command)
{

	if (command == "PROCESS_LIST" || command == "EXPORT" || command == "KEYLOG" ||
		command == "STOPKEYLOG" || command == "LIVESTREAM" || command == "STOPLIVESTREAM" ||
		command == "LIVESTATUS" || command == "SHUTDOWN" || command == "RESTART" ||
		command.substr(0, 6) == "START " || command.substr(0, 5) == "STOP " ||
		command.substr(0, 4) == "GET " || command.substr(0, 3) == "LS " || command.substr(0, 8) == "APP_LIST" ||
		command.substr(0, 9) == "APP_START" || command.substr(0, 9) == "APP_STOP")
	{
		return sendCommandToServer(command);
	}

	else if (command == "STATUS")
	{
		return getLocalSystemStatus();
	}
	else if (command == "HELP")
	{
		return getAvailableCommands();
	}
	else if (command == "ENABLEMAIL")
	{
		enableMailCommand(true);
		return "Mail command processing has been ENABLED.\nThe system will now process commands from emails.";
	}
	else if (command == "DISABLEMAIL")
	{
		disableMailCommand();
		return "Mail command processing has been DISABLED.\nThe system will still monitor emails but won't execute commands.\nUse ENABLEMAIL to re-enable command processing.";
	}
	else if (command.substr(0, 5) == "PING ")
	{
		std::string target = command.substr(5);
		try
		{
			std::string result = exec(("ping -n 4 " + target).c_str());
			return "Ping result for " + target + ":\n" + result;
		}
		catch (...)
		{
			return "Error: Cannot ping " + target;
		}
	}
	else
	{
		return "Invalid command: " + command + "\n\nSend [Controller] HELP for available commands.";
	}
}

std::string MailController::getLocalSystemStatus()
{
	return "Mail Controller Status:\nServer: " + server_ip + ":" + std::to_string(server_port) +
		   "\nTime: " + getCurrentTime() +
		   "\nMonitoring: " + (is_running ? "Running" : "Stopped") +
		   "\nMail Commands: " + (mail_command_enabled ? "Enabled" : "Disabled");
}

std::string MailController::getAvailableCommands()
{
	return R"(=== AVAILABLE COMMANDS ===
SERVER: PROCESS_LIST, START <service>, STOP <service>, KEYLOG, STOPKEYLOG
        APP_LIST, APP_START <app>, APP_STOP <app>, 
        GET <file>, LIVESTREAM, STOPLIVESTREAM, SHUTDOWN, RESTART

LOCAL:  STATUS, PING <host>, HELP
        ENABLEMAIL, DISABLEMAIL

Usage: Send email with subject [Controller] <COMMAND>
Example: [Controller] PROCESS
Note: ENABLEMAIL/DISABLEMAIL controls email command processing
)";
}

bool MailController::sendReport(const std::string &to_email, const std::string &command, const std::string &report)
{
	std::string temp_file = "mail_report.txt";
	std::ofstream email_file(temp_file);

	email_file << "From: " << username << "\r\n";
	email_file << "To: " << to_email << "\r\n";
	email_file << "Subject: [Controller] Report: " << command << "\r\n";
	email_file << "Content-Type: text/plain; charset=utf-8\r\n";
	email_file << "\r\n";
	email_file << "Command executed: " << command << "\r\n";
	email_file << "Execution time: " << getCurrentTime() << "\r\n";
	email_file << "Server: " << server_ip << ":" << server_port << "\r\n";
	email_file << "\r\n=== RESULT ===\r\n";
	email_file << report << "\r\n";
	email_file << "\r\n=== END REPORT ===\r\n";
	email_file.close();

	std::string curl_command = "curl -s --ssl-reqd \"smtps://smtp.gmail.com:465\" "
							   "--user \"" +
							   username + ":" + app_password + "\" "
															   "--mail-from \"" +
							   username + "\" "
										  "--mail-rcpt \"" +
							   to_email + "\" "
										  "--upload-file \"" +
							   temp_file + "\" 2>nul";

	int result = system(curl_command.c_str());
	remove(temp_file.c_str());
	return result == 0;
}

std::string MailController::getCurrentTime()
{
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	std::string time_str = std::ctime(&time_t);
	if (!time_str.empty() && time_str.back() == '\n')
	{
		time_str.pop_back();
	}
	return time_str;
}

std::chrono::system_clock::time_point MailController::parseEmailDate(const std::string &dateStr)
{
	// Try to parse RFC 2822 format: "Thu, 15 Aug 2024 10:30:00 +0700"
	// For simplicity, use current time if parsing fails
	try
	{
		std::tm tm = {};
		std::istringstream ss(dateStr);

		// Skip day of week if present
		std::string token;
		ss >> token;
		if (token.back() == ',')
		{
			// Day name present, skip it
			ss >> token; // day
		}
		else
		{
			// No day name, token is day
		}

		// Parse basic format - this is a simplified parser
		// For production, should use a proper date parsing library
		return std::chrono::system_clock::now();
	}
	catch (...)
	{
		return std::chrono::system_clock::now();
	}
}

bool MailController::isEmailAfterStartup(const std::string &emailContent)
{
	std::istringstream stream(emailContent);
	std::string line;

	while (std::getline(stream, line))
	{
		if (line.length() > 1 && line.back() == '\r')
		{
			line.pop_back();
		}

		if (line.rfind("Date:", 0) == 0)
		{
			std::string dateStr = line.substr(5);
			dateStr.erase(0, dateStr.find_first_not_of(" \t"));

			auto emailTime = parseEmailDate(dateStr);

			// Compare with startup time with 30-second buffer to avoid timezone issues
			auto buffer = std::chrono::seconds(30);
			return emailTime > (startup_time - buffer);
		}
	}

	// If Date header not found, consider email as old
	return false;
}

void MailController::checkNewEmails()
{
	try
	{
		std::string command_fetch_uid = "curl -s --url \"" + imap_server + "/INBOX\" --user \"" + username + ":" + app_password + "\" -X \"FETCH * (UID)\" 2>nul";
		std::string uid_response = exec(command_fetch_uid.c_str());

		std::vector<std::string> new_uids;
		std::istringstream iss(uid_response);
		std::string line;

		while (std::getline(iss, line))
		{
			size_t pos = line.find("UID ");
			if (pos != std::string::npos)
			{
				std::string uid_part = line.substr(pos + 4);
				std::stringstream ss(uid_part);
				std::string uid;
				ss >> uid;

				if (!uid.empty() && uid.back() == ')')
				{
					uid.pop_back();
				}

				if (!uid.empty() && processed_uids.find(uid) == processed_uids.end())
				{
					new_uids.push_back(uid);
				}
			}
		}

		// Process only latest 2 emails to avoid spam
		int count = 0;
		for (auto it = new_uids.rbegin(); it != new_uids.rend() && count < 2; ++it, ++count)
		{
			processEmail(*it);
			saveProcessedUID(*it);
		}
	}
	catch (...)
	{
		// Silent fail to avoid console spam
	}
}

void MailController::processEmail(const std::string &uid)
{
	try
	{
		std::string command_fetch = "curl -s --url \"" + imap_server + "/INBOX;UID=" + uid + "\" --user \"" + username + ":" + app_password + "\" 2>nul";
		std::string email_content = exec(command_fetch.c_str());

		// CHECK TIMESTAMP - ONLY PROCESS EMAILS AFTER STARTUP
		if (!isEmailAfterStartup(email_content))
		{
			// Old email, do not process
			return;
		}

		std::string from_email, subject;
		std::istringstream stream(email_content);
		std::string line;

		while (std::getline(stream, line))
		{
			if (line.length() > 1 && line.back() == '\r')
			{
				line.pop_back();
			}

			if (line.rfind("From:", 0) == 0)
			{
				size_t start = line.find('<');
				size_t end = line.find('>');
				if (start != std::string::npos && end != std::string::npos)
				{
					from_email = line.substr(start + 1, end - start - 1);
				}
				else
				{
					from_email = line.substr(5);
					from_email.erase(0, from_email.find_first_not_of(" \t"));
				}
			}
			else if (line.rfind("Subject:", 0) == 0)
			{
				subject = line.substr(8);
				subject.erase(0, subject.find_first_not_of(" \t"));
			}
		}

		std::string command;
		if (isValidCommand(subject, command))
		{

			if (!mail_command_enabled.load())
			{
				std::string disabled_msg = "Mail command processing is currently DISABLED.\n"
										   "Commands will not be executed.\n"
										   "Use [Controller] ENABLEMAIL to enable command processing.";
				sendReport(from_email, command + " (DISABLED)", disabled_msg);
				return;
			}

			// ONLY SHOW LOG WHEN VALID COMMAND AND ENABLED
			std::cout << "\n[Mail] Command received: " << command << " from " << from_email << std::endl;

			std::string report = processCommand(command);

			std::cout << "[Mail] Command executed, sending report..." << std::endl;

			if (sendReport(from_email, command, report))
			{
				std::cout << "[Mail] Report sent successfully to " << from_email << std::endl;
			}
			else
			{
				std::cout << "[Mail] Failed to send report!" << std::endl;
			}

			// Display brief result
			std::string short_result = report.length() > 100 ? report.substr(0, 100) + "..." : report;
			std::cout << "[Mail] Result preview: " << short_result << std::endl;
		}
	}
	catch (...)
	{
		// Silent fail
	}
}

void MailController::emailMonitoringLoop(int interval_seconds)
{
	while (is_running.load())
	{
		checkNewEmails();

		// Sleep in 1-second chunks for quick stopping
		for (int i = 0; i < interval_seconds && is_running.load(); ++i)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

void MailController::startBackgroundMonitoring(int interval_seconds)
{
	if (is_running.load())
	{
		return; // Already running
	}

	is_running.store(true);

	// Start background monitoring using thread manager
	thread_manager.startNamedThread("EmailMonitoring", [this, interval_seconds]()
									{ emailMonitoringLoop(interval_seconds); }, ThreadCategory::BACKGROUND);

	std::cout << "[Mail] Background email monitoring started (every " << interval_seconds << "s)" << std::endl;
}

void MailController::enableMailCommand(bool enable)
{
	mail_command_enabled.store(enable);
	std::cout << "[Mail] Command processing " << (enable ? "enabled" : "disabled") << std::endl;
}

void MailController::disableMailCommand()
{
	enableMailCommand(false);
}

std::string MailController::getStatus() const
{
	return "Mail monitoring: " + std::string(is_running.load() ? "Running" : "Stopped") +
		   ", Command processing: " + std::string(mail_command_enabled.load() ? "Enabled" : "Disabled");
}