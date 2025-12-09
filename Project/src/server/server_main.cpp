#define NOMINMAX
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <memory>
#include <atomic>
#include <sstream>
#include <vector>
#include "server_control.h"
#include "command_handler.h"
#include "../core/thread_manager.h"
#include "../core/process_manager.h"
#include "../core/keylogger.h"
#include "../core/power_control.h"
#include "../core/livestream.h"
#include "../core/app_manager.h"
#include "../core/screen_capture.h"
#include "../core/constants.h"
#include "../common/string_utils.h"
#include "../common/logger.h"
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

using namespace std;

std::unique_ptr<ServerController> serverController;
std::unique_ptr<ThreadManager> globalThreadManager;
std::unique_ptr<LivestreamServer> livestreamServer;
std::unique_ptr<Logger> serverLogger;
std::unique_ptr<CommandHandler> commandHandler;
ScreenCapture screenCapture;
AppManager appManager;
ProcessManager processManager;
std::unique_ptr<KeyloggerSender> keyloggerServer;

std::atomic<bool> serverRunning(false);

std::vector<std::string> getLocalIPv4Addresses()
{
	std::vector<std::string> ipAddresses;

	ULONG bufferSize = 0;
	DWORD result = GetAdaptersInfo(nullptr, &bufferSize);

	if (result == ERROR_BUFFER_OVERFLOW)
	{
		std::vector<BYTE> buffer(bufferSize);
		PIP_ADAPTER_INFO adapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());

		result = GetAdaptersInfo(adapterInfo, &bufferSize);

		if (result == NO_ERROR)
		{
			PIP_ADAPTER_INFO adapter = adapterInfo;

			while (adapter != nullptr)
			{
				PIP_ADDR_STRING ipAddr = &adapter->IpAddressList;

				while (ipAddr != nullptr)
				{
					std::string ipStr = ipAddr->IpAddress.String;
					if (ipStr != "0.0.0.0" && ipStr != "127.0.0.1")
					{
						ipAddresses.push_back(ipStr);
					}
					ipAddr = ipAddr->Next;
				}

				adapter = adapter->Next;
			}
		}
	}

	return ipAddresses;
}

int main()
{
	std::cout << "=================================================================" << std::endl;
	std::cout << "|              NETWORKING SERVER - REMOTE CONTROL               |" << std::endl;
	std::cout << "=================================================================" << std::endl;

	// Initialize logger
	std::filesystem::create_directories("./server");
	serverLogger = std::make_unique<Logger>("./server/server_log.txt");
	serverLogger->log("MAIN", "=== REMOTE CONTROL SERVER STARTED ===");

	std::vector<std::string> ipAddresses = getLocalIPv4Addresses();
	std::cout << "| Available IPv4 Addresses:                                     |" << std::endl;
	if (ipAddresses.empty())
	{
		std::cout << "|   No active network interfaces found                          |" << std::endl;
	}
	else
	{
		for (const auto &ip : ipAddresses)
		{

			int totalWidth = 63;
			int ipLength = static_cast<int>(ip.length());
			int leftPadding = 5;
			int rightPadding = totalWidth - ipLength - leftPadding;

			std::cout << "|" << std::string(leftPadding, ' ') << ip << std::string(rightPadding, ' ') << "|" << std::endl;
		}
	}
	std::cout << "=================================================================" << std::endl;

	serverLogger->log("INFO", "Starting server initialization...");

	// Initialize global thread manager
	globalThreadManager = std::make_unique<ThreadManager>();
	if (!globalThreadManager->initialize())
	{
		serverLogger->log("ERROR", "Failed to initialize thread manager");
		return 1;
	}

	// Initialize components with thread manager reference
	livestreamServer = std::make_unique<LivestreamServer>(*globalThreadManager);
	keyloggerServer = std::make_unique<KeyloggerSender>(*globalThreadManager);

	serverController = std::make_unique<ServerController>();
	if (!serverController->initializeServer())
	{
		serverLogger->log("ERROR", "Failed to initialize server");
		return 1;
	}

	// Initialize CommandHandler with all required dependencies
	commandHandler = std::make_unique<CommandHandler>(
			serverLogger.get(),
			globalThreadManager.get(),
			&processManager,
			&appManager,
			&screenCapture,
			serverController.get(),
			livestreamServer.get(),
			keyloggerServer.get());

	serverRunning = true;

	// Fix: Add outer loop to keep server running and accept new connections after disconnects
	while (serverRunning.load())
	{
		serverLogger->log("INFO", "Waiting for client connection...");

		if (!serverController->waitForClient())
		{
			serverLogger->log("ERROR", "Failed to connect to client, retrying...");
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		serverLogger->log("INFO", "Client connected - using main thread for commands");

		serverLogger->log("INFO", "Server running:");
		serverLogger->log("INFO", "Command processing - Port " + std::to_string(COMMAND_PORT));
		serverLogger->log("INFO", "Data thread - Port " + std::to_string(DATA_PORT));

		// Process commands in main thread using CommandHandler
		bool clientConnected = true;
		while (clientConnected && serverRunning.load())
		{
			try
			{
				std::string command = serverController->receiveCommand();

				if (command.empty())
				{
					if (!serverController->isClientConnected())
					{
						serverLogger->log("MAIN", "Client disconnected");
						clientConnected = false;
						break;
					}
					continue;
				}

				serverLogger->log("COMMAND", "Processing: " + command);
				std::string reply = commandHandler->processCommand(command);

				// Check if command requires server shutdown
				if (commandHandler->isShutdownCommand(command))
				{
					// serverRunning = false; // Don't shut down server on client exit
				}

				if (!reply.empty())
				{
					serverController->sendCommand(reply);
				}
			}
			catch (const std::exception& e)
			{
				serverLogger->log("ERROR", "Exception in command loop: " + std::string(e.what()));
				// If connection is lost, break inner loop to allow reconnection in outer loop
				if (!serverController->isClientConnected())
				{
					clientConnected = false;
				}
			}
			catch (...)
			{
				serverLogger->log("ERROR", "Unknown exception in command loop");
				clientConnected = false;
			}
		}

		serverController->disconnectClient();

		if (livestreamServer && livestreamServer->isStreaming())
		{
			livestreamServer->stopLivestream();
		}
	}
	
	// Shutdown thread manager and all threads
	if (globalThreadManager)
	{
		globalThreadManager->shutdown();
	}

	serverController->shutdownServer();
	serverLogger->log("INFO", "Server shutdown complete");

	return 0;
}