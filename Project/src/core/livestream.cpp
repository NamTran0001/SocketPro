#include "livestream.h"
#include <iomanip>
#include <filesystem>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

#ifdef _WIN32
#define OPENCV_DISABLE_MSMF_BACKEND
#endif

// Initialize Winsock globally
class WinsockInitializer
{
public:
	WinsockInitializer()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			throw std::runtime_error("WSAStartup failed");
		}
	}
	~WinsockInitializer()
	{
		WSACleanup();
	}
};

// Global Winsock initializer
static WinsockInitializer g_winsockInit;

// SocketWrapper Implementation
void SocketWrapper::close()
{
	if (socket_ != INVALID_SOCKET)
	{
		shutdown(socket_, SD_BOTH);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

bool SocketWrapper::bind(int port)
{
	if (!isValid())
		return false;

	// Set socket options for reusability
	int reuse = 1;
	setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

	int nodelay = 1;
	setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	return ::bind(socket_, (sockaddr *)&addr, sizeof(addr)) != SOCKET_ERROR;
}

bool SocketWrapper::listen(int backlog)
{
	return isValid() && (::listen(socket_, backlog) != SOCKET_ERROR);
}

SocketWrapper SocketWrapper::accept()
{
	if (!isValid())
		return SocketWrapper();

	sockaddr_in clientAddr = {};
	int addrLen = sizeof(clientAddr);
	SOCKET clientSock = ::accept(socket_, (sockaddr *)&clientAddr, &addrLen);

	return SocketWrapper(clientSock);
}

bool SocketWrapper::connect(const std::string &ip, int port, int maxAttempts)
{
	for (int attempt = 1; attempt <= maxAttempts; ++attempt)
	{
		reset(socket(AF_INET, SOCK_STREAM, 0));
		if (!isValid())
			continue;

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
		{
			return false;
		}

		if (::connect(socket_, (sockaddr *)&addr, sizeof(addr)) != SOCKET_ERROR)
		{
			std::cout << "Connected to " << ip << ":" << port << " on attempt " << attempt << std::endl;
			return true;
		}

		std::cout << "Connection attempt " << attempt << "/" << maxAttempts << " failed" << std::endl;
		if (attempt < maxAttempts)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
		}
	}
	return false;
}

// CameraManager Implementation
CameraManager::CameraManager() : isOpen_(false)
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);
}

CameraManager::~CameraManager()
{
	release();
}

bool CameraManager::initialize()
{
	release(); // Ensure clean state

	// Try different camera indices and backends
	for (int camId = 0; camId < MAX_CAMERA_INDEX; ++camId)
	{
		std::cout << "Trying camera " << camId << "..." << std::endl;

		// Try DSHOW first, then MSMF
		for (auto backend : {cv::CAP_DSHOW, cv::CAP_MSMF})
		{
			capture_.open(camId, backend);
			if (capture_.isOpened())
			{
				std::cout << "Camera " << camId << " opened successfully" << std::endl;
				isOpen_ = true;

				// Configure camera settings
				capture_.set(cv::CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
				capture_.set(cv::CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
				capture_.set(cv::CAP_PROP_FPS, CAMERA_FPS);

				return true;
			}
		}
	}

	std::cerr << "Failed to open any camera" << std::endl;
	return false;
}

void CameraManager::release()
{
	if (isOpen_)
	{
		capture_.release();
		cv::destroyAllWindows();
		isOpen_ = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Allow hardware cleanup
	}
}

bool CameraManager::captureFrame(cv::Mat &frame)
{
	if (!isOpened())
		return false;

	for (int attempt = 0; attempt < 3; ++attempt)
	{
		capture_ >> frame;
		if (!frame.empty())
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return false;
}

// LivestreamServer Implementation
LivestreamServer::LivestreamServer(ThreadManager &threadMgr)
		: threadManager_(&threadMgr), state_(ServerState::STOPPED), port_(LIVESTREAM_PORT)
{
	camera_ = std::make_unique<CameraManager>();
}

LivestreamServer::~LivestreamServer()
{
	stopLivestream();
}

void LivestreamServer::changeState(ServerState newState)
{
	state_.store(newState);
	std::cout << "Server state changed to: " << static_cast<int>(newState) << std::endl;
}

bool LivestreamServer::startLivestream(int port)
{
	if (state_.load() != ServerState::STOPPED)
	{
		std::cout << "Server is not in stopped state" << std::endl;
		return false;
	}

	changeState(ServerState::STARTING);
	port_ = port;

	if (!initializeResources())
	{
		changeState(ServerState::STOPPED);
		return false;
	}

	changeState(ServerState::WAITING_FOR_CLIENT);

	threadManager_->startNamedThread("LivestreamServer", [this]()
																	 { streamingLoop(); }, ThreadCategory::NETWORK);

	std::cout << "Livestream server started on port " << port_ << std::endl;
	return true;
}

void LivestreamServer::stopLivestream()
{
	if (state_.load() == ServerState::STOPPED)
		return;

	changeState(ServerState::STOPPING);
	threadManager_->stopNamedThread("LivestreamServer");

	// Cleanup resources
	camera_->release();
	clientSocket_.close();
	serverSocket_.close();

	changeState(ServerState::STOPPED);
	std::cout << "Livestream server stopped" << std::endl;
}

bool LivestreamServer::initializeResources()
{
	// Initialize camera
	if (!camera_->initialize())
	{
		std::cerr << "Failed to initialize camera" << std::endl;
		return false;
	}

	// Initialize server socket
	serverSocket_.reset(socket(AF_INET, SOCK_STREAM, 0));
	if (!serverSocket_.isValid())
	{
		std::cerr << "Failed to create server socket" << std::endl;
		return false;
	}

	if (!serverSocket_.bind(port_) || !serverSocket_.listen())
	{
		std::cerr << "Failed to bind/listen on port " << port_ << std::endl;
		return false;
	}

	std::cout << "Server resources initialized successfully" << std::endl;
	return true;
}

void LivestreamServer::streamingLoop()
{
	// Wait for client connection
	std::cout << "Waiting for client connection..." << std::endl;
	clientSocket_ = serverSocket_.accept();

	if (!clientSocket_.isValid())
	{
		std::cerr << "Failed to accept client connection" << std::endl;
		changeState(ServerState::STOPPED);
		return;
	}

	std::cout << "Client connected successfully" << std::endl;
	changeState(ServerState::STREAMING);

	cv::Mat frame;
	int frameCount = 0;
	int errorCount = 0;
	const int maxErrors = MAX_CONSECUTIVE_ERRORS;

	while (state_.load() == ServerState::STREAMING)
	{
		if (!camera_->captureFrame(frame))
		{
			if (++errorCount >= maxErrors)
			{
				std::cerr << "Too many frame capture errors, stopping" << std::endl;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
			continue;
		}

		errorCount = 0; // Reset error count on successful capture

		if (!sendFrame(frame))
		{
			std::cout << "Client disconnected, stopping stream" << std::endl;
			break;
		}

		if (++frameCount % LOG_FRAME_INTERVAL == 0)
		{
			std::cout << "Streamed " << frameCount << " frames" << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
	}

	std::cout << "Streaming ended. Total frames: " << frameCount << std::endl;
	changeState(ServerState::STOPPED);
}

bool LivestreamServer::sendFrame(const cv::Mat &frame)
{
	if (!clientSocket_.isValid() || frame.empty())
		return false;

	std::vector<uchar> encodedFrame;
	std::vector<int> compressionParams = {cv::IMWRITE_JPEG_QUALITY, JPEG_QUALITY};

	if (!cv::imencode(".jpg", frame, encodedFrame, compressionParams))
	{
		std::cerr << "Failed to encode frame" << std::endl;
		return false;
	}

	int frameSize = static_cast<int>(encodedFrame.size());

	// Send frame size
	if (send(clientSocket_.get(), reinterpret_cast<char *>(&frameSize), sizeof(frameSize), 0) == SOCKET_ERROR)
	{
		return false;
	}

	// Send frame data
	if (send(clientSocket_.get(), reinterpret_cast<char *>(encodedFrame.data()), frameSize, 0) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}

// FrameManager Implementation
void FrameManager::updateFrame(const cv::Mat &frame)
{
	if (frame.empty())
		return;

	std::lock_guard<std::mutex> lock(frameMutex_);
	currentFrame_ = frame.clone();
	frameReady_ = true;
}

cv::Mat FrameManager::getCurrentFrame()
{
	std::lock_guard<std::mutex> lock(frameMutex_);
	if (frameReady_ && !currentFrame_.empty())
	{
		frameReady_ = false;
		return currentFrame_.clone();
	}
	return cv::Mat();
}

void FrameManager::reset()
{
	std::lock_guard<std::mutex> lock(frameMutex_);
	currentFrame_.release();
	currentFrame_ = cv::Mat();
	frameReady_ = false;
}

// VideoRecorder Implementation
VideoRecorder::VideoRecorder() : isRecording_(false) {}

VideoRecorder::~VideoRecorder()
{
	stopRecording();
}

bool VideoRecorder::startRecording(const cv::Size &frameSize)
{
	if (isRecording_)
		return false;

	// Generate timestamped filename
	auto now = std::chrono::system_clock::now();
	auto timeT = std::chrono::system_clock::to_time_t(now);
	auto localTm = std::localtime(&timeT);

	std::ostringstream filename;
	filename << std::put_time(localTm, "%Y%m%d_%H%M%S") << ".avi";

	std::filesystem::path clientDir = std::filesystem::current_path() / "client";
	std::filesystem::create_directories(clientDir);
	outputPath_ = (clientDir / filename.str()).string();

	std::lock_guard<std::mutex> lock(writerMutex_);
	writer_ = std::make_unique<cv::VideoWriter>();

	int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
	if (writer_->open(outputPath_, codec, CAMERA_FPS, frameSize))
	{
		isRecording_ = true;
		std::cout << "Recording started: " << outputPath_ << std::endl;
		return true;
	}

	std::cerr << "Failed to start video recording" << std::endl;
	return false;
}

void VideoRecorder::recordFrame(const cv::Mat &frame)
{
	if (!isRecording_ || frame.empty())
		return;

	std::lock_guard<std::mutex> lock(writerMutex_);
	if (writer_ && writer_->isOpened())
	{
		writer_->write(frame);
	}
}

void VideoRecorder::stopRecording()
{
	if (!isRecording_)
		return;

	std::lock_guard<std::mutex> lock(writerMutex_);
	if (writer_ && writer_->isOpened())
	{
		writer_->release();
		std::cout << "Recording finished: " << outputPath_ << std::endl;
	}
	writer_.reset();
	isRecording_ = false;
}

// LivestreamClient Implementation
LivestreamClient::LivestreamClient(ThreadManager &threadMgr)
		: threadManager_(&threadMgr), state_(ClientState::DISCONNECTED),
			port_(DATA_PORT), captureFrames_(false), displayEnabled_(true)
{

	frameManager_ = std::make_unique<FrameManager>();
	videoRecorder_ = std::make_unique<VideoRecorder>();
}

LivestreamClient::~LivestreamClient()
{
	stopReceiving();
}

void LivestreamClient::changeState(ClientState newState)
{
	state_.store(newState);
	std::cout << "Client state changed to: " << static_cast<int>(newState) << std::endl;
}

bool LivestreamClient::startReceiving(const std::string &serverIp, int port)
{
	if (state_.load() != ClientState::DISCONNECTED)
	{
		std::cout << "Client is not in disconnected state" << std::endl;
		return false;
	}

	serverIp_ = serverIp;
	port_ = port;

	reset(); // Ensure clean state

	changeState(ClientState::CONNECTING);

	if (!establishConnection())
	{
		changeState(ClientState::DISCONNECTED);
		return false;
	}

	changeState(ClientState::CONNECTED);

	threadManager_->startNamedThread("LivestreamReceiver", [this]()
																	 { receivingLoop(); }, ThreadCategory::NETWORK);

	std::cout << "Client started receiving from " << serverIp << ":" << port << std::endl;
	return true;
}

void LivestreamClient::stopReceiving()
{
	if (state_.load() == ClientState::DISCONNECTED)
		return;

	changeState(ClientState::DISCONNECTING);
	threadManager_->stopNamedThread("LivestreamReceiver");

	videoRecorder_->stopRecording();
	clientSocket_.close();
	cv::destroyAllWindows();

	changeState(ClientState::DISCONNECTED);
	std::cout << "Client stopped receiving" << std::endl;
}

bool LivestreamClient::establishConnection()
{
	return clientSocket_.connect(serverIp_, port_);
}

void LivestreamClient::receivingLoop()
{
	changeState(ClientState::RECEIVING);

	cv::Mat frame;
	int frameCount = 0;

	std::cout << "Starting receiving loop" << std::endl;

	while (state_.load() == ClientState::RECEIVING)
	{
		if (!receiveFrame(frame))
		{
			std::cout << "Failed to receive frame, stopping" << std::endl;
			break;
		}

		processReceivedFrame(frame);

		if (++frameCount % 60 == 0)
		{
			std::cout << "Received " << frameCount << " frames" << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	std::cout << "Receiving ended. Total frames: " << frameCount << std::endl;
	changeState(ClientState::DISCONNECTED);
}

bool LivestreamClient::receiveFrame(cv::Mat &frame)
{
	if (!clientSocket_.isValid())
		return false;

	// Receive frame size
	int frameSize;
	int bytesReceived = recv(clientSocket_.get(), reinterpret_cast<char *>(&frameSize), sizeof(frameSize), 0);
	if (bytesReceived <= 0 || frameSize <= 0 || frameSize > MAX_FRAME_SIZE)
	{
		return false;
	}

	// Receive frame data
	std::vector<uchar> buffer(frameSize);
	int totalReceived = 0;

	while (totalReceived < frameSize)
	{
		bytesReceived = recv(clientSocket_.get(),
												 reinterpret_cast<char *>(buffer.data()) + totalReceived,
												 frameSize - totalReceived, 0);
		if (bytesReceived <= 0)
			return false;
		totalReceived += bytesReceived;
	}

	// Decode frame
	frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
	return !frame.empty();
}

void LivestreamClient::processReceivedFrame(const cv::Mat &frame)
{
	// Update display frame
	frameManager_->updateFrame(frame);

	// Record to video if enabled
	if (videoRecorder_->isRecording())
	{
		videoRecorder_->recordFrame(frame);
	}

	// Capture frames if enabled
	if (captureFrames_)
	{
		std::lock_guard<std::mutex> lock(capturedFramesMutex_);
		capturedFrames_.push_back(frame.clone());
	}
}

void LivestreamClient::reset()
{
	displayEnabled_ = true;
	captureFrames_ = false;
	frameManager_->reset();

	std::lock_guard<std::mutex> lock(capturedFramesMutex_);
	capturedFrames_.clear();
}

std::vector<cv::Mat> LivestreamClient::getCapturedFrames()
{
	std::lock_guard<std::mutex> lock(capturedFramesMutex_);
	std::vector<cv::Mat> frames = capturedFrames_;
	capturedFrames_.clear();
	return frames;
}

void LivestreamClient::enableRecording(bool enable)
{
	if (enable && !videoRecorder_->isRecording())
	{
		// Start recording with dummy frame size - will be adjusted on first frame
		videoRecorder_->startRecording(cv::Size(CAMERA_WIDTH, CAMERA_HEIGHT));
	}
	else if (!enable && videoRecorder_->isRecording())
	{
		videoRecorder_->stopRecording();
	}
}

std::string LivestreamClient::getCurrentVideoPath() const
{
	return videoRecorder_->getOutputPath();
}

void LivestreamClient::startDisplayInMainThread()
{
	if (!displayEnabled_ || state_.load() != ClientState::RECEIVING)
	{
		std::cout << "Display not started - not enabled or not receiving" << std::endl;
		return;
	}

	cv::namedWindow("Client - Livestream", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
	cv::resizeWindow("Client - Livestream", CAMERA_WIDTH, CAMERA_HEIGHT);

	int frameCount = 0;

	while (state_.load() == ClientState::RECEIVING && displayEnabled_)
	{
		if (hasNewFrame())
		{
			cv::Mat frame = getCurrentFrame();
			if (!frame.empty())
			{
				cv::imshow("Client - Livestream", frame);
				frameCount++;
			}
		}

		int key = cv::waitKey(30) & 0xFF;
		if (key == 27)
		{ // ESC key
			std::cout << "ESC pressed, stopping stream" << std::endl;
			if (commandCallback_)
			{
				commandCallback_("STOPLIVESTREAM");
			}
			break;
		}

		if (cv::getWindowProperty("Client - Livestream", cv::WND_PROP_VISIBLE) < 1)
		{
			std::cout << "Window closed, stopping stream" << std::endl;
			if (commandCallback_)
			{
				commandCallback_("STOPLIVESTREAM");
			}
			break;
		}
	}

	std::cout << "Display ended. Frames displayed: " << frameCount << std::endl;
}

void LivestreamClient::stopDisplayInMainThread()
{
	displayEnabled_ = false;
	cv::destroyAllWindows();
}
