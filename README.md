# 🕵️ NET-GHOST v2.0 - Điều Khiển & Giám Sát Hệ Thống Từ Xa

<div align="center">

![Status](https://img.shields.io/badge/status-production%20ready-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue)
![Python](https://img.shields.io/badge/python-3.8+-blue)
![C++](https://img.shields.io/badge/C++-17-blue)
![License](https://img.shields.io/badge/license-Educational-orange)

**Nền tảng quản lý hệ thống từ xa chuyên nghiệp với khả năng giám sát và điều khiển real-time**

</div>

---

## 📋 Mục Lục

- [Tổng quan](#-tổng-quan)
- [Tính năng](#-tính-năng)
- [Kiến trúc](#-kiến-trúc)
- [Yêu cầu hệ thống](#-yêu-cầu-hệ-thống)
- [Khởi động nhanh](#-khởi-động-nhanh)
- [Cài đặt](#-cài-đặt)
- [Cấu hình](#-cấu-hình)
- [API Reference](#-api-reference)
- [Phát triển](#-phát-triển)
- [Xử lý sự cố](#-xử-lý-sự-cố)
- [Bảo mật](#-cân-nhắc-bảo-mật)
- [Cải tiến tương lai](#-cải-tiến-tương-lai)

---

## 🌟 Tổng quan

NET-GHOST là một **hệ thống quản trị từ xa hai tầng** mạnh mẽ, kết hợp giao diện web Python Flask với backend C++ hiệu năng cao. Hệ thống cung cấp khả năng điều khiển toàn diện, giám sát real-time và streaming media thông qua dashboard web tinh tế.

### 🎯 Trạng thái dự án

| Component | Trạng thái | Chi tiết |
|-----------|--------|---------|
| **C++ Backend** | ✅ Production Ready | Build với CMake + VS2022 MSVC++, C++17 |
| **Flask Web UI** | ✅ Production Ready | Dashboard hiện đại responsive với Tailwind CSS |
| **Tính năng Real-time** | ✅ Hoạt động đầy đủ | SSE keylogger, MJPEG streaming (webcam/màn hình) |
| **Kiến trúc** | ✅ Tuân thủ | Theo đặc tả Architecture.md |
| **Network Protocol** | ✅ Ổn định | TCP persistent connections với chunked transfer |
| **Dependencies** | ✅ Đã giải quyết | OpenCV 4.11.0, Flask, psutil, Windows SDK |

### 🔑 Điểm nổi bật

- 🌐 **Giao diện web hiện đại** - Dashboard sạch đẹp với cập nhật real-time mỗi 3 giây
- 🚀 **Hiệu năng cao** - Persistent TCP connections, backend C++ đa luồng
- 📹 **Media Streaming** - Giám sát webcam & màn hình với MJPEG protocol (30-60 FPS)
- ⌨️ **Live Keylogger** - Streaming keystroke real-time qua Server-Sent Events (SSE)
- 💻 **Điều khiển hệ thống** - Quản lý process, khởi động ứng dụng, điều khiển nguồn
- 📊 **Giám sát tài nguyên** - CPU, RAM, disk, network stats với biểu đồ trực quan
- 🔌 **Linh hoạt protocol** - 4 cổng TCP chuyên dụng cho các thao tác khác nhau

---

## ✨ Tính năng

### 🖥️ Quản lý hệ thống
- **Process Monitor** - Xem tất cả processes đang chạy với PID, tên, memory usage, CPU usage
- **Kill Processes** - Ngắt processes theo PID hoặc tên ứng dụng
- **Điều khiển ứng dụng** - Khởi động/dừng ứng dụng đã cài đặt từ xa
- **Quản lý nguồn** - Lệnh shutdown, restart hệ thống
- **System Stats** - Giám sát real-time CPU, RAM, disk usage, network I/O

### 📸 Media Capture & Streaming
- **Screenshot Capture** - Chụp màn hình toàn màn hình theo yêu cầu (JPEG, 1920x1080)
- **Live Screen Monitoring** - Streaming màn hình real-time (~10 FPS, MJPEG)
- **Webcam Streaming** - Streaming video HD (có thể cấu hình 30-60 FPS)
- **Video Recording** - Ghi webcam streams thành file AVI với timestamps
- **Auto-save Screenshots** - Tự động lưu trữ screenshots vào thư mục `screenshots/`

### ⌨️ Keylogging & Monitoring
- **Real-time Keylogger** - Capture keystroke trực tiếp qua Server-Sent Events
- **Session Statistics** - Theo dõi tổng số phím bấm, thời lượng session, hoạt động cuối
- **Terminal Display** - Trình xem keylog kiểu console với auto-scroll
- **Điều khiển Start/Stop** - Bật/tắt keylogger từ giao diện web

### 📊 Dashboard & Analytics
- **Resource Graphs** - Chỉ báo trực quan CPU/RAM usage
- **Network Stats** - Tốc độ upload/download, tổng dữ liệu truyền tải
- **Recent Apps** - Shortcuts khởi động nhanh cho ứng dụng thường dùng
- **Uptime Tracking** - Thời gian khởi động và uptime của hệ thống
- **Connection Status** - Giám sát server health real-time

### 🔧 Tính năng nâng cao
- **Persistent Connections** - Socket connection pooling cho độ trễ thấp
- **Auto-reconnect** - Tự động thử lại khi mất kết nối
- **Multi-tab Interface** - Điều hướng sidebar có tổ chức (Dashboard, Processes, Apps, Screen, Webcam, Keylog)
- **Responsive Design** - Giao diện thân thiện mobile với Tailwind CSS
- **Dark Theme** - UI chuyên nghiệp lấy cảm hứng từ cyberpunk với màu cyan

---

## 🏗️ Kiến trúc

### Thiết kế hệ thống
