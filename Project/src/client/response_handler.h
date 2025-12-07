#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "../core/thread_manager.h"
#include "../common/logger.h"
#include "client_control.h"
#include "../core/livestream.h"

// ResponseHandler class for processing server responses using ThreadManager
class ResponseHandler
{
private:
	ThreadManager &threadManager;
	Logger &logger;
	ClientController &clientController;
	LivestreamClient &livestreamClient;
	std::filesystem::path clientPath;

public:
	ResponseHandler(ThreadManager &tm, Logger &log, ClientController &cc,
									LivestreamClient &lc, const std::filesystem::path &path);
	~ResponseHandler() = default;

	// Main method to process commands using ThreadManager
	void processCommand(const std::string &command);

private:
	// Specialized handlers for different command types
	void handleScreenCapture();
	void handleStopLivestream();
	void handleFileDownload(const std::string &command);

	// Chunked data handlers with progress callback
	void handleProcessList();
	void handleAppList();
	void handleDirectoryListing(const std::string &command);

	// Helper methods
	void displayProcessList(const std::string &csvData);
	void displayAppList(const std::string &csvData);
	void saveScreenshot(const std::vector<uint8_t> &imageData);
	void convertFramesToVideo(const std::vector<cv::Mat> &frames);
	bool downloadFile(const std::string &filepath);

	// Progressive display callbacks for chunked data
	void processListProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes);
	void appListProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes);
	void dirListProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes);
	void fileDownloadProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes);

private:
	// Accumulated data for chunked transfers
	std::string accumulatedProcessData;
	std::string accumulatedAppData;
	std::string accumulatedDirData;
};
