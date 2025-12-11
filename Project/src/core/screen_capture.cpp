#include "screen_capture.h"
#include "constants.h"
#include <iostream>
#include <algorithm>

#include <Windows.h>

// Khai báo các hằng số và hàm nếu chưa có trong phiên bản SDK của bạn
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT) - 4)
#endif

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

HBITMAP ScreenCapture::captureScreen(int x, int y, int width, int height)
{
	if (width <= 0 || height <= 0 || width > MAX_SCREEN_DIMENSION || height > MAX_SCREEN_DIMENSION)
	{
		std::cerr << "Error: Invalid capture dimensions: " << width << "x" << height << std::endl;
		return NULL;
	}

	HDC hScreenDC = GetDC(nullptr);
	if (hScreenDC == NULL)
	{
		std::cerr << "Error: Failed to get screen DC" << std::endl;
		return NULL;
	}

	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	if (hMemoryDC == NULL)
	{
		std::cerr << "Error: Failed to create memory DC" << std::endl;
		ReleaseDC(nullptr, hScreenDC);
		return NULL;
	}

	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
	if (hBitmap == NULL)
	{
		std::cerr << "Error: Failed to create compatible bitmap" << std::endl;
		DeleteDC(hMemoryDC);
		ReleaseDC(nullptr, hScreenDC);
		return NULL;
	}

	HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemoryDC, hBitmap));

	if (!BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY))
	{
		std::cerr << "Error: BitBlt failed" << std::endl;
		SelectObject(hMemoryDC, hOldBitmap);
		DeleteObject(hBitmap);
		DeleteDC(hMemoryDC);
		ReleaseDC(nullptr, hScreenDC);
		return NULL;
	}

	SelectObject(hMemoryDC, hOldBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(nullptr, hScreenDC);

	return hBitmap;
}

cv::Mat ScreenCapture::hbitmapToMat(HBITMAP hbitmap)
{
	if (hbitmap == NULL)
	{
		std::cerr << "Error: NULL HBITMAP passed to hbitmapToMat" << std::endl;
		return cv::Mat();
	}

	BITMAP bmp;
	if (GetObject(hbitmap, sizeof(BITMAP), &bmp) == 0)
	{
		std::cerr << "Error: Failed to get BITMAP object info" << std::endl;
		DeleteObject(hbitmap);
		return cv::Mat();
	}

	// std::cout << "[ScreenCapture] BITMAP info - Width: " << bmp.bmWidth
	// 					<< ", Height: " << bmp.bmHeight
	// 					<< ", BitsPixel: " << bmp.bmBitsPixel << std::endl;

	int channels = bmp.bmBitsPixel / 8;
	if (channels != 3 && channels != 4)
	{
		std::cerr << "Error: Unsupported color depth: " << bmp.bmBitsPixel << " bits" << std::endl;
		DeleteObject(hbitmap);
		return cv::Mat();
	}

	cv::Mat mat(bmp.bmHeight, bmp.bmWidth, channels == 4 ? CV_8UC4 : CV_8UC3);

	BITMAPINFOHEADER bi;
	ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = bmp.bmBitsPixel;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	HDC hdc = GetDC(nullptr);
	if (hdc == NULL)
	{
		std::cerr << "Error: Failed to get device context" << std::endl;
		DeleteObject(hbitmap);
		return cv::Mat();
	}

	int result = GetDIBits(hdc, hbitmap, 0, bmp.bmHeight, mat.data,
												 reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

	ReleaseDC(nullptr, hdc);
	DeleteObject(hbitmap);

	if (result == 0)
	{
		std::cerr << "Error: GetDIBits failed" << std::endl;
		return cv::Mat();
	}

	if (channels == 4)
	{
		cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
	}

	// std::cout << "[ScreenCapture] Successfully converted HBITMAP to Mat" << std::endl;

	return mat;
}

cv::Mat ScreenCapture::captureFullScreen()
{
	try
	{
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// std::cout << "[ScreenCapture] Screen dimensions: " << screenWidth << "x" << screenHeight << std::endl;

		if (screenWidth <= 0 || screenHeight <= 0)
		{
			std::cerr << "Error: Invalid screen dimensions: "
								<< screenWidth << "x" << screenHeight << std::endl;
			return cv::Mat();
		}

		if (screenWidth > MAX_SCREEN_DIMENSION || screenHeight > MAX_SCREEN_DIMENSION)
		{
			std::cerr << "Error: Screen dimensions too large: "
								<< screenWidth << "x" << screenHeight << std::endl;
			return cv::Mat();
		}

		// std::cout << "[ScreenCapture] Attempting to capture screen..." << std::endl;

		HBITMAP hbitmap = captureScreen(0, 0, screenWidth, screenHeight);
		if (hbitmap == NULL)
		{
			std::cerr << "Error: Failed to capture screen bitmap" << std::endl;
			return cv::Mat();
		}

		// std::cout << "[ScreenCapture] Screen bitmap captured, converting to Mat..." << std::endl;

		cv::Mat result = hbitmapToMat(hbitmap);
		if (result.empty())
		{
			std::cerr << "Error: Failed to convert bitmap to Mat" << std::endl;
			return cv::Mat();
		}

		// std::cout << "[ScreenCapture] Conversion successful, Mat size: "
		// 					<< result.cols << "x" << result.rows << ", channels: " << result.channels() << std::endl;

		return result;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception in captureFullScreen: " << e.what() << std::endl;
		return cv::Mat();
	}
	catch (...)
	{
		std::cerr << "Unknown exception in captureFullScreen" << std::endl;
		return cv::Mat();
	}
}

cv::Mat ScreenCapture::captureMonitor(int monitorIndex)
{
	// Enumerate monitors and capture specific one
	struct MonitorData
	{
		int index;
		int currentIndex;
		RECT bounds;
	} data = {monitorIndex, 0, {0}};

	EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL
											{
            auto* data = reinterpret_cast<MonitorData*>(lParam);
            if (data->currentIndex == data->index) {
                MONITORINFO info;
                info.cbSize = sizeof(MONITORINFO);
                GetMonitorInfo(hMonitor, &info);
                data->bounds = info.rcMonitor;
                return FALSE;
            }
            data->currentIndex++;
            return TRUE; }, reinterpret_cast<LPARAM>(&data));

	int width = data.bounds.right - data.bounds.left;
	int height = data.bounds.bottom - data.bounds.top;

	HBITMAP hbitmap = captureScreen(data.bounds.left, data.bounds.top, width, height);
	return hbitmapToMat(hbitmap);
}

cv::Mat ScreenCapture::captureWindow(HWND windowHandle)
{
	RECT rect;
	GetWindowRect(windowHandle, &rect);

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	// Bring window to front temporarily
	HWND previousForeground = GetForegroundWindow();
	SetForegroundWindow(windowHandle);
	Sleep(100); // Wait for window to come to front

	HBITMAP hbitmap = captureScreen(rect.left, rect.top, width, height);

	// Restore previous foreground window
	SetForegroundWindow(previousForeground);

	return hbitmapToMat(hbitmap);
}

cv::Mat ScreenCapture::captureRegion(int x, int y, int width, int height)
{
	HBITMAP hbitmap = captureScreen(x, y, width, height);
	return hbitmapToMat(hbitmap);
}

std::vector<uint8_t> ScreenCapture::encodeToJPEG(const cv::Mat &image, int quality)
{
	std::vector<uint8_t> buffer;
	std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
	cv::imencode(".jpg", image, buffer, params);
	return buffer;
}

std::vector<uint8_t> ScreenCapture::encodeToPNG(const cv::Mat &image, int compression)
{
	std::vector<uint8_t> buffer;
	std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, compression};
	cv::imencode(".png", image, buffer, params);
	return buffer;
}

std::vector<std::string> ScreenCapture::getWindowList()
{
	std::vector<std::string> windows;

	EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
							{
        if (IsWindowVisible(hwnd)) {
            char title[256];
            GetWindowTextA(hwnd, title, sizeof(title));
            if (strlen(title) > 0) {
                auto* windows = reinterpret_cast<std::vector<std::string>*>(lParam);
                windows->push_back(title);
            }
        }
        return TRUE; }, reinterpret_cast<LPARAM>(&windows));

	return windows;
}

HWND ScreenCapture::findWindowByTitle(const std::string &title)
{
	return FindWindowA(nullptr, title.c_str());
}

std::pair<int, int> ScreenCapture::getScreenResolution()
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	return {width, height};
}