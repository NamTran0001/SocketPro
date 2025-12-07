#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <memory>
#include <functional>
#include "constants.h"
#include "thread_manager.h"

// RAII Socket wrapper for automatic cleanup
class SocketWrapper
{
private:
	SOCKET socket_;

public:
	SocketWrapper() : socket_(INVALID_SOCKET) {}
	explicit SocketWrapper(SOCKET sock) : socket_(sock) {}
	~SocketWrapper() { close(); }

	// Move semantics only
	SocketWrapper(const SocketWrapper &) = delete;
	SocketWrapper &operator=(const SocketWrapper &) = delete;
	SocketWrapper(SocketWrapper &&other) noexcept : socket_(other.socket_)
	{
		other.socket_ = INVALID_SOCKET;
	}
	SocketWrapper &operator=(SocketWrapper &&other) noexcept
	{
		if (this != &other)
		{
			close();
			socket_ = other.socket_;
			other.socket_ = INVALID_SOCKET;
		}
		return *this;
	}

	SOCKET get() const { return socket_; }
	bool isValid() const { return socket_ != INVALID_SOCKET; }
	void reset(SOCKET sock = INVALID_SOCKET)
	{
		close();
		socket_ = sock;
	}
	void close();

	// Socket operations
	bool bind(int port);
	bool listen(int backlog = 1);
	SocketWrapper accept();
	bool connect(const std::string &ip, int port, int maxAttempts = MAX_CONNECTION_ATTEMPTS);
};

// Camera manager with RAII
class CameraManager
{
private:
	cv::VideoCapture capture_;
	bool isOpen_;

public:
	CameraManager();
	~CameraManager();

	bool initialize();
	void release();
	bool isOpened() const { return isOpen_ && capture_.isOpened(); }
	bool captureFrame(cv::Mat &frame);
	cv::VideoCapture &getCapture() { return capture_; }
};

// Simplified server state
enum class ServerState
{
	STOPPED,
	STARTING,
	WAITING_FOR_CLIENT,
	STREAMING,
	STOPPING
};

class LivestreamServer
{
private:
	SocketWrapper serverSocket_;
	SocketWrapper clientSocket_;
	std::unique_ptr<CameraManager> camera_;
	ThreadManager *threadManager_;
	std::atomic<ServerState> state_;
	int port_;

	bool initializeResources();
	void streamingLoop();
	bool sendFrame(const cv::Mat &frame);
	void changeState(ServerState newState);

public:
	LivestreamServer(ThreadManager &threadMgr);
	~LivestreamServer();

	bool startLivestream(int port = LIVESTREAM_PORT);
	void stopLivestream();
	bool isStreaming() const { return state_.load() == ServerState::STREAMING; }
	ServerState getState() const { return state_.load(); }
};

// Frame manager for client-side frame handling
class FrameManager
{
private:
	cv::Mat currentFrame_;
	std::mutex frameMutex_;
	std::atomic<bool> frameReady_;

public:
	FrameManager() : frameReady_(false) {}

	void updateFrame(const cv::Mat &frame);
	cv::Mat getCurrentFrame();
	bool hasNewFrame() const { return frameReady_.load(); }
	void reset();
};

// Video recorder with RAII
class VideoRecorder
{
private:
	std::unique_ptr<cv::VideoWriter> writer_;
	std::string outputPath_;
	std::mutex writerMutex_;
	bool isRecording_;

public:
	VideoRecorder();
	~VideoRecorder();

	bool startRecording(const cv::Size &frameSize);
	void recordFrame(const cv::Mat &frame);
	void stopRecording();
	std::string getOutputPath() const { return outputPath_; }
	bool isRecording() const { return isRecording_; }
};

// Simplified client state
enum class ClientState
{
	DISCONNECTED,
	CONNECTING,
	CONNECTED,
	RECEIVING,
	DISCONNECTING
};

class LivestreamClient
{
private:
	SocketWrapper clientSocket_;
	ThreadManager *threadManager_;
	std::atomic<ClientState> state_;
	std::string serverIp_;
	int port_;

	// Component managers
	std::unique_ptr<FrameManager> frameManager_;
	std::unique_ptr<VideoRecorder> videoRecorder_;

	// Simplified state flags
	std::atomic<bool> captureFrames_;
	std::atomic<bool> displayEnabled_;
	std::vector<cv::Mat> capturedFrames_;
	std::mutex capturedFramesMutex_;

	// Callback for commands
	std::function<void(const std::string &)> commandCallback_;

	bool establishConnection();
	void receivingLoop();
	bool receiveFrame(cv::Mat &frame);
	void processReceivedFrame(const cv::Mat &frame);
	void changeState(ClientState newState);

public:
	LivestreamClient(ThreadManager &threadMgr);
	~LivestreamClient();

	bool startReceiving(const std::string &serverIp, int port = DATA_PORT);
	void stopReceiving();
	bool isReceiving() const { return state_.load() == ClientState::RECEIVING; }
	ClientState getState() const { return state_.load(); }

	// Frame capture control
	void enableFrameCapture(bool enable) { captureFrames_ = enable; }
	std::vector<cv::Mat> getCapturedFrames();

	// Recording control
	void enableRecording(bool enable);
	std::string getCurrentVideoPath() const;

	// Display control
	void enableDisplay(bool enable) { displayEnabled_ = enable; }
	bool hasNewFrame() const { return frameManager_->hasNewFrame(); }
	cv::Mat getCurrentFrame() { return frameManager_->getCurrentFrame(); }
	void startDisplayInMainThread();
	void stopDisplayInMainThread();

	// Command callback
	void setCommandCallback(std::function<void(const std::string &)> callback)
	{
		commandCallback_ = callback;
	}

	// Complete reset for reusability
	void reset();
};