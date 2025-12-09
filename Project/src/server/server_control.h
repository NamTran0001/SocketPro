#pragma once

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include "constants.h"

using std::cout, std::endl;

// Data transfer header structure
struct DataChunkHeader
{
	DataTransferCommand command; // Command type using enum (2 bytes)
	uint16_t reserved1;					 // Alignment padding (2 bytes)
	uint32_t totalChunks;				 // Total number of chunks (4 bytes)
	uint32_t currentChunk;			 // Current chunk index 0-based (4 bytes)
	uint32_t chunkSize;					 // Size of current chunk data (4 bytes)
	uint32_t totalSize;					 // Total data size (4 bytes)
	uint32_t checksum;					 // CRC32 checksum of chunk data (4 bytes)
															 // Total header size: 24 bytes (reduced from 32 bytes)
};

// Progress callback function type
// Parameters: (data_buffer, current_chunk, total_chunks, bytes_transferred, total_bytes)
using ProgressCallback = std::function<void(const char *, uint32_t, uint32_t, size_t, size_t)>;

struct DualSocket
{
	SOCKET commandSocket; // For command communication
	SOCKET dataSocket;		// For file/data transfer
	bool isActive;

	DualSocket() : commandSocket(INVALID_SOCKET), dataSocket(INVALID_SOCKET), isActive(false) {}
};

class ServerController
{
private:
	WSADATA wsa;
	SOCKET serverCommandSocket;
	SOCKET serverDataSocket;
	DualSocket clientSockets;
	sockaddr_in serverAddr;
	sockaddr_in clientAddr;

	std::atomic<bool> running;
	std::mutex socketMutex;

public:
	ServerController();
	~ServerController();

	bool initializeServer();
	bool waitForClient();
	void disconnectClient();
	void shutdownServer();

	// Command socket operations
	bool sendCommand(const std::string &command);
	std::string receiveCommand();

	// Data socket operations
	bool sendData(const void *data, size_t size, DataTransferCommand command = DataTransferCommand::DATA, const ProgressCallback &progressCallback = nullptr);
	bool sendFile(const std::string &filePath, const ProgressCallback &progressCallback = nullptr);

	// Socket access
	SOCKET getCommandSocket() { return clientSockets.commandSocket; }
	SOCKET getDataSocket() { return clientSockets.dataSocket; }
	bool isClientConnected() { return clientSockets.isActive; }
	bool isServerRunning() { return running; }

private:
	// Helper functions for chunked data transfer
	uint32_t calculateCRC32(const void *data, size_t size);
	bool sendChunk(const char *data, size_t size, DataTransferCommand command, uint32_t currentChunk, uint32_t totalChunks, size_t totalSize);
};
