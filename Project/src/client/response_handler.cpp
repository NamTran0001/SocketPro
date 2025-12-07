#include "response_handler.h"
#include "../core/constants.h"
#include "../common/string_utils.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>

ResponseHandler::ResponseHandler(ThreadManager &tm, Logger &log, ClientController &cc,
																 LivestreamClient &lc, const std::filesystem::path &path)
		: threadManager(tm), logger(log), clientController(cc), livestreamClient(lc), clientPath(path)
{
}

void ResponseHandler::processCommand(const std::string &command)
{
	logger.log("RESPONSE", "Processing command: " + command);
	try
	{
		if (!clientController.isConnected())
		{
			logger.log("RESPONSE", "Not connected to server");
			return;
		}

		cout << "Sending command: " << command << endl;
		if (!clientController.sendCommand(command))
		{
			logger.log("RESPONSE", "Failed to send command");
			return;
		}

		// Handle commands that use chunked data transfer
		if (command == "PROCESS_LIST")
		{
			handleProcessList();
		}
		else if (command == "APP_LIST")
		{
			handleAppList();
		}
		else if (command == "LS" || command.rfind("LS ", 0) == 0)
		{
			handleDirectoryListing(command);
		}
		else if (command.rfind("GET ", 0) == 0)
		{
			// Fix: Read response from command socket FIRST
			std::string response = clientController.receiveCommand();
			if (response == "CHUNKED_SUCCESS")
			{
				handleFileDownload(command);
			}
			else
			{
				logger.log("RESPONSE", "GET command failed: " + response);
				std::cout << "[ERROR]: " << response << std::endl;
			}
		}
		else if (command == "SCREEN_CAPTURE")
		{
			std::string response = clientController.receiveCommand();
			if (response == "CHUNKED_SUCCESS")
			{
				handleScreenCapture();
			}
			else
			{
				logger.log("RESPONSE", "SCREEN_CAPTURE command failed: " + response);
				std::cout << "[ERROR]: " << response << std::endl;
			}
		}
		else
		{
			// Handle other commands with standard response
			std::string response = clientController.receiveCommand();
			if (!response.empty())
			{
				logger.log("RESPONSE", "Response received: " + response);

				// Handle ACK and ERROR responses
				if (response.rfind("ACK:", 0) == 0 || response.rfind("ERROR:", 0) == 0)
				{
					std::cout << "[SERVER]: " << response << std::endl;
					logger.log("COMMAND_RESULT", command + " - " + response);
				}
				else
				{
					// Handle specific commands that require special processing
					if (command == "STOPLIVESTREAM")
					{
						handleStopLivestream();
					}
					else
					{
						// Display generic response for other commands
						std::cout << "[SERVER]: " << response << std::endl;
						logger.log("COMMAND_RESULT", command + " - " + response);
					}
				}
			}
		}

		cout << "Press Enter to continue..." << endl;
	}
	catch (const std::exception &e)
	{
		logger.log("RESPONSE", "Error processing command: " + std::string(e.what()));
	}
}

void ResponseHandler::handleScreenCapture()
{
	logger.log("RESPONSE", "Receiving screen capture with chunked transfer");

	auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
	{
		double progressPercent = (static_cast<double>(bytesTransferred) / totalBytes) * 100.0;
		std::cout << "\r[SCREEN_CAPTURE] Progress: [";

		int barWidth = 40;
		int pos = static_cast<int>(barWidth * progressPercent / 100.0);

		for (int i = 0; i < barWidth; ++i)
		{
			if (i < pos)
				std::cout << "=";
			else if (i == pos)
				std::cout << ">";
			else
				std::cout << " ";
		}

		std::cout << "] " << std::fixed << std::setprecision(1) << progressPercent << "% ";
		std::cout << "(" << bytesTransferred << "/" << totalBytes << " bytes)";
		std::cout.flush();

		if (currentChunk == totalChunks)
		{
			std::cout << std::endl;
		}
	};

	auto receivedData = clientController.receiveData(progressCallback);

	if (!receivedData.empty())
	{
		saveScreenshot(receivedData);
		std::cout << "Screen capture saved successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to receive screen capture data." << std::endl;
		logger.log("ERROR", "No screen capture data received");
	}
}

void ResponseHandler::handleStopLivestream()
{
	logger.log("LIVESTREAM", "Processing STOPLIVESTREAM command");

	if (livestreamClient.isReceiving())
	{
		// Get current video path before stopping
		std::string currentVideoPath = livestreamClient.getCurrentVideoPath();

		// Stop the livestream (this will finalize real-time recording)
		livestreamClient.stopReceiving(); // This clears any captured frames

		if (!currentVideoPath.empty())
		{
			logger.log("LIVESTREAM", "Real-time video saved: " + currentVideoPath);
			std::cout << "[LIVESTREAM]: Real-time video saved: " << currentVideoPath << std::endl;
		}
		else
		{
			logger.log("LIVESTREAM", "No real-time video was created");
			std::cout << "[LIVESTREAM]: No video file was created during streaming" << std::endl;
		}
	}
	else
	{
		logger.log("LIVESTREAM", "Livestream was not running");
		std::cout << "[LIVESTREAM]: No active livestream to stop" << std::endl;
	}
}

void ResponseHandler::handleFileDownload(const std::string &command)
{
	logger.log("DEBUG", "Starting file download for command: " + command);

	std::string filepath = command.substr(4); // Remove "GET " prefix
	filepath = trimString(filepath);
	if (!filepath.empty() && filepath.front() == '"' && filepath.back() == '"')
	{
		filepath = filepath.substr(1, filepath.length() - 2);
	}

	logger.log("DEBUG", "Parsed filepath: " + filepath);

	std::string outputPath = (clientPath / std::filesystem::path(filepath).filename()).string();
	logger.log("DEBUG", "Output path: " + outputPath);

	if (downloadFile(outputPath))
	{
		logger.log("RESPONSE", "File downloaded: " + outputPath);
		std::cout << "[DATA]: File saved as " << outputPath << std::endl;
	}
	else
	{
		logger.log("RESPONSE", "Failed to download file: " + filepath);
		std::cout << "[ERROR]: Failed to download file" << std::endl;
	}
}

void ResponseHandler::displayProcessList(const std::string &csvData)
{
	// Clear screen and reposition cursor for progressive updates
	if (!csvData.empty())
	{
		std::cout << "\033[2J\033[1;1H"; // Clear screen and move to top
	}

	std::cout << "\n=== PROCESS LIST ===" << std::endl;
	std::istringstream ss(csvData);
	std::string line;
	int displayIndex = 1;
	bool isHeader = true;

	while (std::getline(ss, line))
	{
		if (line.empty())
			continue;

		// Display header
		if (isHeader && line.find("Name") != std::string::npos)
		{
			std::cout << "No. | Process Name              | PID    | Session Name | Session# | Memory Usage" << std::endl;
			std::cout << "----+---------------------------+--------+--------------+----------+-------------" << std::endl;
			isHeader = false;
			continue;
		}

		if (!isHeader && line.find(',') != std::string::npos)
		{
			// Parse CSV line for display
			std::vector<std::string> fields;
			std::istringstream lineStream(line);
			std::string field;

			while (std::getline(lineStream, field, ','))
			{
				// Remove quotes
				if (!field.empty() && field.front() == '"')
					field.erase(0, 1);
				if (!field.empty() && field.back() == '"')
					field.pop_back();
				fields.push_back(field);
			}

			if (fields.size() >= 5)
			{
				std::cout << std::setw(3) << displayIndex << " | "
									<< std::setw(25) << std::left << fields[0].substr(0, 25) << " | "
									<< std::setw(6) << std::right << fields[1] << " | "
									<< std::setw(12) << std::left << fields[2].substr(0, 12) << " | "
									<< std::setw(8) << std::right << fields[3] << " | "
									<< std::setw(11) << std::left << fields[4] << std::endl;
				displayIndex++;

				if (displayIndex > LIMIT_PROCESS_DISPLAY)
					break;
			}
		}
	}
	std::cout << "---------------------------------------------------------------------------------" << std::endl;
}

void ResponseHandler::displayAppList(const std::string &csvData)
{
	// Clear screen and reposition cursor for progressive updates
	if (!csvData.empty())
	{
		std::cout << "\033[2J\033[1;1H"; // Clear screen and move to top
	}

	std::cout << "\n=== APPLICATION LIST ===" << std::endl;
	std::istringstream ss(csvData);
	std::string line;
	int displayIndex = 1;
	bool isHeader = true;

	while (std::getline(ss, line))
	{
		if (line.empty())
			continue;

		// Display header
		if (isHeader && line.find("No.") != std::string::npos)
		{
			std::cout << "No. | Application Name                    | Version      | Installation Path" << std::endl;
			std::cout << "----+-------------------------------------+--------------+------------------" << std::endl;
			isHeader = false;
			continue;
		}

		if (!isHeader && line.find(',') != std::string::npos)
		{
			// Parse CSV line for display
			std::vector<std::string> fields;
			std::istringstream lineStream(line);
			std::string field;

			while (std::getline(lineStream, field, ','))
			{
				// Remove quotes
				if (!field.empty() && field.front() == '"')
					field.erase(0, 1);
				if (!field.empty() && field.back() == '"')
					field.pop_back();
				fields.push_back(field);
			}

			if (fields.size() >= 4)
			{
				std::cout << std::setw(3) << fields[0] << " | "
									<< std::setw(35) << std::left << fields[1].substr(0, 35) << " | "
									<< std::setw(12) << std::left << fields[2].substr(0, 12) << " | "
									<< std::left << fields[3].substr(0, 60) << std::endl;

				if (displayIndex > LIMIT_APP_DISPLAY)
					break;
				displayIndex++;
			}
		}
	}
	std::cout << "---------------------------------------------------------------------------------" << std::endl;
}

void ResponseHandler::saveScreenshot(const std::vector<uint8_t> &imageData)
{
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream filename;
	filename << "screenshot_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".jpg";

	std::filesystem::path fullPath = clientPath / filename.str();

	std::ofstream imageFile(fullPath, std::ios::binary);
	if (imageFile.is_open())
	{
		imageFile.write(reinterpret_cast<const char *>(imageData.data()), imageData.size());
		imageFile.close();
		logger.log("RESPONSE", "Screenshot saved: " + fullPath.string());
		std::cout << "[DATA]: Screenshot saved as " << fullPath.string() << std::endl;
	}
}

void ResponseHandler::convertFramesToVideo(const std::vector<cv::Mat> &frames)
{
	if (frames.empty())
	{
		logger.log("LIVESTREAM", "No frames to convert to video");
		std::cout << "[ERROR]: No frames available for video conversion" << std::endl;
		return;
	}

	// Generate timestamp in UTC+7 (Vietnam timezone)
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);

	// Convert to UTC+7
	std::tm *local_tm = std::localtime(&time_t);
	std::mktime(local_tm); // Normalize the time structure

	std::stringstream filename;
	filename << std::put_time(local_tm, "%Y%m%d_%H%M%S") << ".avi";

	std::filesystem::path fullPath = clientPath / filename.str();

	// Ensure output directory exists
	std::filesystem::create_directories(clientPath);

	cv::VideoWriter videoWriter;
	cv::Size frameSize = frames[0].size();

	// Use MJPEG codec for AVI format - reliable and widely supported
	int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
	double fps = 30.0;

	if (videoWriter.open(fullPath.string(), codec, fps, frameSize, true))
	{
		logger.log("LIVESTREAM", "Writing " + std::to_string(frames.size()) + " frames to video");

		for (size_t i = 0; i < frames.size(); ++i)
		{
			if (!frames[i].empty())
			{
				videoWriter.write(frames[i]);
			}

			// Progress indicator
			if (i % 30 == 0)
			{
				std::cout << "\rWriting video: " << std::fixed << std::setprecision(1)
									<< (static_cast<double>(i) / frames.size() * 100.0) << "%" << std::flush;
			}
		}

		videoWriter.release();
		std::cout << "\r[LIVESTREAM]: Video saved as " << fullPath.string()
							<< " (" << frames.size() << " frames)" << std::endl;
		logger.log("LIVESTREAM", "AVI video created successfully: " + fullPath.string());
	}
	else
	{
		logger.log("LIVESTREAM", "Failed to create video writer with MJPEG codec");
		std::cout << "[ERROR]: Failed to create video file - codec not supported" << std::endl;
	}
}

bool ResponseHandler::downloadFile(const std::string &outputPath)
{
	auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
	{
		this->fileDownloadProgressCallback(data, currentChunk, totalChunks, bytesTransferred, totalBytes);
	};

	return clientController.receiveFile(outputPath, progressCallback);
}

// New chunked data handlers
void ResponseHandler::handleProcessList()
{
	logger.log("RESPONSE", "Receiving process list with chunked transfer");
	accumulatedProcessData.clear();

	auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
	{
		this->processListProgressCallback(data, currentChunk, totalChunks, bytesTransferred, totalBytes);
	};

	auto receivedData = clientController.receiveData(progressCallback);

	if (!receivedData.empty())
	{
		std::string csvData(receivedData.begin(), receivedData.end());
		logger.log("RESPONSE", "Process list received successfully");

		// Save CSV data to file
		std::ofstream csvFile(clientPath / "process_list.csv");
		if (csvFile.is_open())
		{
			csvFile << csvData;
			csvFile.close();
			logger.log("RESPONSE", "Process list saved to process_list.csv");
			std::cout << "Process list saved to ./client/process_list.csv" << std::endl;
		}
	}
	else
	{
		logger.log("RESPONSE", "Failed to receive process list");
	}
}

void ResponseHandler::handleAppList()
{
	logger.log("RESPONSE", "Receiving app list with chunked transfer");
	accumulatedAppData.clear();

	auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
	{
		this->appListProgressCallback(data, currentChunk, totalChunks, bytesTransferred, totalBytes);
	};

	auto receivedData = clientController.receiveData(progressCallback);

	if (!receivedData.empty())
	{
		std::string csvData(receivedData.begin(), receivedData.end());
		logger.log("RESPONSE", "App list received successfully");

		// Save CSV data to file
		std::ofstream csvFile(clientPath / "app_list.csv");
		if (csvFile.is_open())
		{
			csvFile << csvData;
			csvFile.close();
			logger.log("RESPONSE", "App list saved to app_list.csv");
			std::cout << "Application list saved to ./client/app_list.csv" << std::endl;
		}
	}
	else
	{
		logger.log("RESPONSE", "Failed to receive app list");
	}
}

void ResponseHandler::handleDirectoryListing(const std::string &command)
{
	logger.log("RESPONSE", "Receiving directory listing with chunked transfer");
	accumulatedDirData.clear();

	auto progressCallback = [this](const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
	{
		this->dirListProgressCallback(data, currentChunk, totalChunks, bytesTransferred, totalBytes);
	};

	auto receivedData = clientController.receiveData(progressCallback);

	if (!receivedData.empty())
	{
		std::string dirData(receivedData.begin(), receivedData.end());
		logger.log("RESPONSE", "Directory listing received successfully");

		// Display directory listing
		std::cout << "\n=== DIRECTORY LISTING ===" << std::endl;
		std::istringstream ss(dirData);
		std::string line;
		int index = 0;

		while (std::getline(ss, line))
		{
			if (!line.empty())
			{
				if (!index)
					std::cout << line << std::endl;
				else
					std::cout << index << ". " << line << std::endl;
				index++;
			}
		}
	}
	else
	{
		logger.log("RESPONSE", "Failed to receive directory listing");
	}
}

// Progress callback implementations
void ResponseHandler::processListProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
{
	// Accumulate data for progressive display
	if (data && currentChunk > 0)
	{
		accumulatedProcessData.append(data);
	}

	// Show progress bar
	double progressPercent = (static_cast<double>(bytesTransferred) / totalBytes) * 100.0;
	std::cout << "\r[PROCESS_LIST] Progress: [";

	int barWidth = 30;
	int pos = static_cast<int>(barWidth * progressPercent / 100.0);

	for (int i = 0; i < barWidth; ++i)
	{
		if (i < pos)
			std::cout << "=";
		else if (i == pos)
			std::cout << ">";
		else
			std::cout << " ";
	}

	std::cout << "] " << std::fixed << std::setprecision(1) << progressPercent << "% ";
	std::cout << "(" << currentChunk << "/" << totalChunks << " chunks)";
	std::cout.flush();

	if (currentChunk == totalChunks)
	{
		std::cout << std::endl
							<< "[PROCESS_LIST] Transfer complete!" << std::endl;
		displayProcessList(accumulatedProcessData);
	}
}

void ResponseHandler::appListProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
{
	// Accumulate data for progressive display
	if (data && currentChunk > 0)
	{
		accumulatedAppData.append(data);
	}

	// Show progress bar
	double progressPercent = (static_cast<double>(bytesTransferred) / totalBytes) * 100.0;
	std::cout << "\r[APP_LIST] Progress: [";

	int barWidth = 30;
	int pos = static_cast<int>(barWidth * progressPercent / 100.0);

	for (int i = 0; i < barWidth; ++i)
	{
		if (i < pos)
			std::cout << "=";
		else if (i == pos)
			std::cout << ">";
		else
			std::cout << " ";
	}

	std::cout << "] " << std::fixed << std::setprecision(1) << progressPercent << "% ";
	std::cout << "(" << currentChunk << "/" << totalChunks << " chunks)";
	std::cout.flush();

	if (currentChunk == totalChunks)
	{
		std::cout << std::endl
							<< "[APP_LIST] Transfer complete!" << std::endl;
		displayAppList(accumulatedAppData);
	}
}

void ResponseHandler::dirListProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
{
	// Accumulate data for progressive display
	if (data && currentChunk > 0)
	{
		accumulatedDirData.append(data);
	}

	// Show progress bar
	double progressPercent = (static_cast<double>(bytesTransferred) / totalBytes) * 100.0;
	std::cout << "\r[DIRECTORY] Progress: [";

	int barWidth = 30;
	int pos = static_cast<int>(barWidth * progressPercent / 100.0);

	for (int i = 0; i < barWidth; ++i)
	{
		if (i < pos)
			std::cout << "=";
		else if (i == pos)
			std::cout << ">";
		else
			std::cout << " ";
	}

	std::cout << "] " << std::fixed << std::setprecision(1) << progressPercent << "% ";
	std::cout << "(" << currentChunk << "/" << totalChunks << " chunks)";
	std::cout.flush();

	if (currentChunk == totalChunks)
	{
		std::cout << std::endl
							<< "[DIRECTORY] Transfer complete!" << std::endl;
	}
}

void ResponseHandler::fileDownloadProgressCallback(const char *data, uint32_t currentChunk, uint32_t totalChunks, size_t bytesTransferred, size_t totalBytes)
{
	// Show file download progress bar
	double progressPercent = (static_cast<double>(bytesTransferred) / totalBytes) * 100.0;

	// Calculate transfer speed
	static auto startTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);

	double speed = 0.0;
	if (duration.count() > 0)
	{
		speed = (static_cast<double>(bytesTransferred) / 1024.0) / (duration.count() / 1000.0); // KB/s
	}

	std::cout << "\r[FILE_DOWNLOAD] Progress: [";

	int barWidth = 40;
	int pos = static_cast<int>(barWidth * progressPercent / 100.0);

	for (int i = 0; i < barWidth; ++i)
	{
		if (i < pos)
			std::cout << "=";
		else if (i == pos)
			std::cout << ">";
		else
			std::cout << " ";
	}

	std::cout << "] " << std::fixed << std::setprecision(1) << progressPercent << "% ";
	std::cout << "(" << bytesTransferred << "/" << totalBytes << " bytes) ";
	std::cout << std::fixed << std::setprecision(2) << speed << " KB/s";
	std::cout.flush();

	if (currentChunk == totalChunks)
	{
		std::cout << std::endl
							<< "[FILE_DOWNLOAD] Transfer complete!" << std::endl;
		startTime = std::chrono::steady_clock::now(); // Reset for next transfer
	}
}
