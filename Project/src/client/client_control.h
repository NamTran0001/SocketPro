#pragma once

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include "constants.h"

using namespace std;

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

struct ClientDualSocket
{
	SOCKET commandSocket; // For command communication
	SOCKET dataSocket;		// For file/data transfer
	bool isConnected;

	ClientDualSocket() : commandSocket(INVALID_SOCKET), dataSocket(INVALID_SOCKET), isConnected(false) {}
};

class ClientController
{
private:
	WSADATA wsa;
	ClientDualSocket sockets;
	sockaddr_in serverAddr;
	std::string serverIP;

	std::atomic<bool> connected;
	std::mutex socketMutex;

public:
	ClientController();
	~ClientController();

	bool initialize();
	bool connectToServer(const std::string &ip);
	void disconnect();

	// Command socket operations
	bool sendCommand(const std::string &command);
	std::string receiveCommand();

	// Data socket operations
	std::vector<uint8_t> receiveData(const ProgressCallback &progressCallback = nullptr);
	bool receiveFile(const std::string &outputPath, const ProgressCallback &progressCallback = nullptr);

	// Socket access
	SOCKET getCommandSocket() { return sockets.commandSocket; }
	SOCKET getDataSocket() { return sockets.dataSocket; }
	bool isConnected() { return sockets.isConnected; }

private:
	// Helper functions for chunked data transfer
	uint32_t calculateCRC32(const void *data, size_t size);
	bool receiveChunk(DataChunkHeader &header, std::vector<uint8_t> &chunkData);
};
