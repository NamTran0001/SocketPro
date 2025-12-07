#pragma once

#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>

class ScreenCapture
{
private:
	// Helper methods
	HBITMAP captureScreen(int x, int y, int width, int height);
	cv::Mat hbitmapToMat(HBITMAP hbitmap);

public:
	// Capture methods
	cv::Mat captureFullScreen();
	cv::Mat captureMonitor(int monitorIndex = 0);
	cv::Mat captureWindow(HWND windowHandle);
	cv::Mat captureRegion(int x, int y, int width, int height);

	// Encoding methods
	std::vector<uint8_t> encodeToJPEG(const cv::Mat &image, int quality);
	std::vector<uint8_t> encodeToPNG(const cv::Mat &image, int compression);

	// Utility methods
	std::vector<std::string> getWindowList();
	HWND findWindowByTitle(const std::string &title);
	std::pair<int, int> getScreenResolution();
};