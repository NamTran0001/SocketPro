# 🕵️ NET-GHOST v2.0 - Điều Khiển & Giám Sát Hệ Thống Từ Xa

<div align="center">

![Status](https://img.shields.io/badge/status-production%20ready-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue)
![Python](https://img.shields.io/badge/python-3.8+-blue)
![C++](https://img.shields.io/badge/C++-17-blue)
![License](https://img.shields.io/badge/license-Educational-orange)

**Nền tảng quản trị hệ thống từ xa chuyên nghiệp với khả năng giám sát và điều khiển real-time**

[Tính Năng](#-tính-năng) • [Bắt Đầu Nhanh](#-bắt-đầu-nhanh) • [Tài Liệu](#-tài-liệu) • [API Reference](#-api-reference) • [Kiến Trúc](#-kiến-trúc)

</div>

---

## 📋 Mục Lục

- [Tổng Quan](#-tổng-quan)
- [Tính Năng](#-tính-năng)
- [Kiến Trúc](#-kiến-trúc)
- [Yêu Cầu Hệ Thống](#-yêu-cầu-hệ-thống)
- [Bắt Đầu Nhanh](#-bắt-đầu-nhanh)
- [Cài Đặt](#-cài-đặt)
- [Cấu Hình](#-cấu-hình)
- [API Reference](#-api-reference)
- [Phát Triển](#-phát-triển)
- [Khắc Phục Sự Cố](#-khắc-phục-sự-cố)
- [Bảo Mật](#-bảo-mật)
- [Cải Tiến Tương Lai](#-cải-tiến-tương-lai)

---

## 🌟 Tổng Quan

NET-GHOST là một **hệ thống quản trị từ xa hai tầng** mạnh mẽ, kết hợp giao diện web Flask Python với backend C++ hiệu năng cao. Cung cấp khả năng điều khiển hệ thống toàn diện, giám sát real-time và streaming media qua dashboard web đẹp mắt.

### 🎯 Tình Trạng Dự Án

| Component | Trạng Thái | Chi Tiết |
|-----------|-----------|---------|
| **C++ Backend** | ✅ Sẵn Sàng Production | Build bằng CMake + VS2022 MSVC++, C++17 |
| **Flask Web UI** | ✅ Sẵn Sàng Production | Dashboard responsive hiện đại với Tailwind CSS |
| **Tính Năng Real-time** | ✅ Hoạt Động Đầy Đủ | SSE keylogger, MJPEG streaming (webcam/screen) |
| **Kiến Trúc** | ✅ Tuân Thủ | Theo đúng đặc tả Architecture.md |
| **Network Protocol** | ✅ Ổn Định | TCP persistent connections với chunked transfer |
| **Dependencies** | ✅ Đã Giải Quyết | OpenCV 4.11.0, Flask, psutil, Windows SDK |

### 🔑 Điểm Nổi Bật

- 🌐 **Giao Diện Web Hiện Đại** - Dashboard sạch đẹp với cập nhật real-time mỗi 3 giây
- 🚀 **Hiệu Năng Cao** - Persistent TCP connections, backend C++ đa luồng
- 📹 **Media Streaming** - Giám sát webcam & màn hình với MJPEG protocol (30-60 FPS)
- ⌨️ **Live Keylogger** - Streaming keystroke real-time qua Server-Sent Events (SSE)
- 💻 **Điều Khiển Hệ Thống** - Quản lý process, khởi chạy ứng dụng, điều khiển nguồn
- 📊 **Giám Sát Tài Nguyên** - Thống kê CPU, RAM, disk, network với biểu đồ trực quan
- 🔌 **Protocol Linh Hoạt** - 4 cổng TCP chuyên biệt cho các thao tác khác nhau

---

## ✨ Tính Năng

### 🖥️ Quản Lý Hệ Thống
- **Giám Sát Process** - Xem tất cả process đang chạy với PID, tên, memory usage, CPU usage
- **Kill Process** - Kết thúc process theo PID hoặc tên ứng dụng
- **Điều Khiển Ứng Dụng** - Start/stop các ứng dụng đã cài đặt từ xa
- **Quản Lý Nguồn** - Lệnh shutdown, restart hệ thống
- **Thống Kê Hệ Thống** - Giám sát real-time CPU, RAM, disk usage, network I/O

### 📸 Chụp & Streaming Media
- **Chụp Ảnh Màn Hình** - Screenshot full-screen theo yêu cầu (JPEG, 1920x1080)
- **Giám Sát Màn Hình Trực Tiếp** - Screen streaming real-time (~10 FPS, MJPEG)
- **Webcam Streaming** - Video streaming HD (cấu hình được 30-60 FPS)
- **Ghi Hình Video** - Ghi webcam streams thành file AVI với timestamp
- **Tự Động Lưu Screenshot** - Lưu screenshot tự động vào thư mục `screenshots/`

### ⌨️ Keylogging & Giám Sát
- **Real-time Keylogger** - Capture keystroke trực tiếp qua Server-Sent Events
- **Thống Kê Session** - Theo dõi tổng số phím nhấn, thời lượng session, hoạt động cuối
- **Hiển Thị Terminal** - Trình xem keylog kiểu console với auto-scroll
- **Điều Khiển Start/Stop** - Bật/tắt keylogger từ giao diện web

### 📊 Dashboard & Phân Tích
- **Biểu Đồ Tài Nguyên** - Chỉ báo trực quan CPU/RAM usage
- **Thống Kê Mạng** - Tốc độ upload/download, tổng data transferred
- **Ứng Dụng Gần Đây** - Phím tắt khởi chạy nhanh cho ứng dụng phổ biến
- **Theo Dõi Uptime** - Thời gian boot hệ thống và thời lượng uptime
- **Trạng Thái Kết Nối** - Giám sát health server real-time

### 🔧 Tính Năng Nâng Cao
- **Persistent Connections** - Socket connection pooling cho độ trễ thấp
- **Tự Động Kết Nối Lại** - Retry tự động khi connection bị lỗi
- **Giao Diện Đa Tab** - Điều hướng sidebar có tổ chức (Dashboard, Processes, Apps, Screen, Webcam, Keylog)
- **Thiết Kế Responsive** - Giao diện thân thiện với mobile bằng Tailwind CSS
- **Dark Theme** - UI phong cách cyberpunk chuyên nghiệp với điểm nhấn cyan

---

## 🏗️ Kiến Trúc

### Thiết Kế Hệ Thống

```
┌─────────────────────────────────────────────────────────────────┐
│                         CLIENT (Browser)                         │
│                    http://localhost:5001                         │
└────────────────────────────┬────────────────────────────────────┘
                             │ HTTP/SSE/MJPEG
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    FLASK WEB SERVER (Python)                     │
│  ┌──────────────┬──────────────┬──────────────┬──────────────┐  │
│  │   REST API   │  SSE Stream  │ MJPEG Stream │  Templates   │  │
│  │ /api/control │  /keylog     │  /video_feed │ dashboard.html│ │
│  └──────────────┴──────────────┴──────────────┴──────────────┘  │
│                                                                   │
│  Socket Client (Persistent Connections)                          │
└──────┬─────────────┬─────────────┬─────────────┬────────────────┘
       │ TCP 8888    │ TCP 8889    │ TCP 8890    │ TCP 8891
       │ COMMAND     │ DATA        │ LIVESTREAM  │ KEYLOGGER
       ▼             ▼             ▼             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    C++ SERVER (Backend)                          │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Command Handler Thread (Main Loop)                       │   │
│  │  • Xử lý ASCII commands                                   │   │
│  │  • Route tới handlers phù hợp                             │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────┬──────────────┬──────────────┬──────────────┐  │
│  │ Process Mgr  │ Screen Cap   │ Livestream   │  Keylogger   │  │
│  │ • List procs │ • Screenshot │ • OpenCV cam │ • Win hooks  │  │
│  │ • Kill task  │ • JPEG enc   │ • MJPEG enc  │ • SSE stream │  │
│  └──────────────┴──────────────┴──────────────┴──────────────┘  │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Thread Manager (Worker Pool)                             │   │
│  │  • Xử lý data async                                       │   │
│  │  • Truyền file chunked                                    │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                             │
                             ▼
                    ┌────────────────┐
                    │  Windows OS    │
                    │  • WinAPI      │
                    │  • Processes   │
                    │  • Registry    │
                    └────────────────┘
```

### Đặc Tả Network Protocol

| Port | Channel | Protocol | Mục Đích | Định Dạng Dữ Liệu |
|------|---------|----------|---------|-------------------|
| **8888** | COMMAND | TCP Text | Gửi commands, nhận ACK | Chuỗi ASCII |
| **8889** | DATA | TCP Binary | Payload lớn (CSV, images) | 24-byte header + chunks |
| **8890** | LIVESTREAM | TCP Binary | Video streaming | 4-byte size + JPEG frames |
| **8891** | KEYLOGGER | TCP Text | Keystroke streaming | Text lines với timestamps |

#### Data Channel Protocol (Port 8889)

**Header Format** (24 bytes, big-endian):
```python
HEADER_FORMAT = '>HHIIIII'
# Các trường:
# - cmd (uint16)        : Mã command
# - res (uint16)        : Reserved
# - total (uint32)      : Tổng số chunks
# - curr (uint32)       : Chỉ số chunk hiện tại (0-based)
# - size (uint32)       : Kích thước payload chunk hiện tại (bytes)
# - total_size (uint32) : Tổng kích thước payload qua tất cả chunks
# - checksum (uint32)   : CRC32 checksum (tùy chọn)
```

**Luồng Truyền**:
1. Flask gửi command qua COMMAND_PORT (ví dụ: `"SCREEN_CAPTURE"`)
2. C++ server xác nhận trên COMMAND_PORT
3. C++ server gửi data chunks qua DATA_PORT:
   - Mỗi chunk: [24-byte header][payload bytes]
   - Lặp cho đến khi `curr >= total - 1`
4. Flask tập hợp chunks thành payload hoàn chỉnh

#### Livestream Protocol (Port 8890)

**Frame Format**:
```
[4 bytes: int32 frame_size][frame_size bytes: JPEG data]
[4 bytes: int32 frame_size][frame_size bytes: JPEG data]
...
```

MJPEG streaming với HTTP multipart response dựa trên boundary:
```http
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg

[JPEG binary data]
--frame
Content-Type: image/jpeg

[JPEG binary data]
...
```

---

## 🛠️ Yêu Cầu Hệ Thống

### Yêu Cầu Hệ Thống

| Component | Yêu Cầu | Ghi Chú |
|-----------|---------|---------|
| **Hệ Điều Hành** | Windows 10/11 x64 | C++ server yêu cầu Windows APIs |
| **Python** | 3.8 trở lên | Đã test trên Python 3.10+ |
| **Compiler** | MSVC++ (VS 2022) | Để build C++ server |
| **CMake** | 3.20 trở lên | Build system |
| **RAM** | Tối thiểu 4GB, khuyến nghị 8GB | Để streaming mượt mà |
| **Network** | Kết nối local hoặc LAN | Các cổng TCP 8888-8891 phải truy cập được |

### Dependencies Phần Mềm

#### C++ Server (Backend)
- **Visual Studio 2022 Community** (hoặc Professional/Enterprise)
  - Workload: "Desktop development with C++"
  - Components: MSVC v143, Windows SDK, CMake tools
- **OpenCV 4.11.0**
  - Download: [opencv.org](https://opencv.org/releases/)
  - Đường dẫn cài đặt: `C:\opencv` (hoặc cấu hình CMake tương ứng)
  - Modules bắt buộc: core, imgcodecs, videoio, imgproc

#### Flask Web UI (Frontend)
```bash
pip install flask>=3.0.0
pip install psutil>=5.9.6
pip install opencv-python>=4.11.0
pip install numpy>=1.26.2
```

### Cấu Hình Network
- **Firewall**: Cho phép inbound connections trên các cổng 8888-8891
- **Router**: Port forwarding nếu truy cập từ mạng ngoài (không khuyến nghị vì lý do bảo mật)
- **Antivirus**: Có thể cần whitelist `server.exe` và Python scripts

---

## 🚀 Bắt Đầu Nhanh

### 1️⃣ Khởi Động C++ Server (Backend)

```powershell
# Chuyển đến thư mục dự án
cd c:\python\Projects\Socket\SocketPro

# Build server (nếu chưa build)
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target server

# Chạy server
.\build\Debug\server.exe
```

**Kết quả mong đợi:**
```
[SERVER] Starting NET-GHOST Server v2.0...
[NETWORK] IPv4 Addresses:
  - 127.0.0.1 (Loopback)
  - 192.168.1.100 (Ethernet)
[COMMAND] Listening on 0.0.0.0:8888
[DATA] Listening on 0.0.0.0:8889
[LIVESTREAM] Listening on 0.0.0.0:8890
[KEYLOGGER] Listening on 0.0.0.0:8891
[SERVER] Ready to accept connections
```

### 2️⃣ Khởi Động Flask Web UI

```bash
# Kích hoạt virtual environment (nếu dùng)
venv\Scripts\activate

# Chạy Flask application
python app.py
```

**Kết quả mong đợi:**
```
 * Serving Flask app 'app'
 * Debug mode: on
 * Running on http://0.0.0.0:5001 (Press CTRL+C to quit)
[DEBUG] Creating persistent socket connections...
[DEBUG] Command socket connected
[DEBUG] Data socket connected
```

### 3️⃣ Truy Cập Dashboard

Mở browser và truy cập:
```
http://localhost:5001
```

🎉 **Bạn sẽ thấy dashboard NET-GHOST với thống kê hệ thống!**

---

## 📦 Cài Đặt

### Hướng Dẫn Cài Đặt Chi Tiết

#### Bước 1: Clone Repository

```bash
git clone https://github.com/yourusername/SocketPro.git
cd SocketPro
```

#### Bước 2: Cài Đặt C++ Dependencies

1. **Cài Đặt Visual Studio 2022 Community**
   - Download: [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
   - Trong quá trình cài đặt, chọn: "Desktop development with C++"
   - Đảm bảo CMake tools được bao gồm

2. **Cài Đặt OpenCV 4.11.0**
   ```powershell
   # Download OpenCV từ opencv.org
   # Giải nén vào C:\opencv\
   
   # Thêm vào PATH (PowerShell Admin)
   $env:PATH += ";C:\opencv\build\x64\vc16\bin"
   
   # Xác minh cài đặt
   dir C:\opencv\build\x64\vc16\bin\opencv_world4110.dll
   ```

3. **Cấu Hình OpenCV cho CMake**
   ```powershell
   # Đặt environment variable
   [System.Environment]::SetEnvironmentVariable(
       "OpenCV_DIR",
       "C:\opencv\build",
       [System.EnvironmentVariableTarget]::User
   )
   ```

#### Bước 3: Build C++ Server

```powershell
# Cấu hình CMake project
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DOpenCV_DIR=C:\opencv\build

# Build tất cả targets
cmake --build build --config Debug

# Hoặc build target cụ thể
cmake --build build --config Debug --target server

# Xác minh build
dir build\Debug\server.exe
```

#### Bước 4: Thiết Lập Python Environment

```bash
# Tạo virtual environment (khuyến nghị)
python -m venv venv

# Kích hoạt environment
# Windows:
venv\Scripts\activate
# Linux/Mac:
source venv/bin/activate

# Cài đặt dependencies
pip install -r requirements.txt

# Hoặc cài thủ công:
pip install flask psutil opencv-python numpy
```

#### Bước 5: Cấu Hình Kết Nối

Chỉnh sửa [app.py](app.py) để đặt IP server:

```python
# Line 15: Đặt địa chỉ IP của máy C++ server
VM_IP = "127.0.0.1"  # Máy local
# VM_IP = "192.168.1.100"  # Máy từ xa trên LAN
```

#### Bước 6: Tạo Thư Mục Cần Thiết

```powershell
# Flask sẽ tự tạo, nhưng bạn có thể chuẩn bị trước:
New-Item -ItemType Directory -Force -Path screenshots
New-Item -ItemType Directory -Force -Path recordings
New-Item -ItemType Directory -Force -Path templates
```

#### Bước 7: Chạy Ứng Dụng

```powershell
# Terminal 1: Khởi động C++ server
.\build\Debug\server.exe

# Terminal 2: Khởi động Flask UI
python app.py
```

---

## ⚙️ Cấu Hình

### Cấu Hình C++ Server

Chỉnh sửa [src/core/constants.h](src/core/constants.h):

```cpp
// ===== CẤU HÌNH NETWORK =====
constexpr int COMMAND_PORT = 8888;      // Command channel
constexpr int DATA_PORT = 8889;         // Truyền data lớn
constexpr int LIVESTREAM_PORT = 8890;   // Video streaming
constexpr int KEYLOGGER_PORT = 8891;    // Keylogger stream

// ===== KÍCH THƯỚC BUFFER & CHUNK =====
constexpr int COMMAND_BUFFER_SIZE = 4096;          // 4KB
constexpr int DATA_CHUNK_SIZE = 1024 * 1024;       // 1MB mỗi chunk
constexpr int MAX_FRAME_SIZE = 10000000;           // Max 10MB frame
constexpr size_t MAX_ALLOWED_FILE_SIZE = 500 * 1024 * 1024; // 500MB

// ===== CÀI ĐẶT CAMERA =====
constexpr int CAMERA_WIDTH = 1280;      // Độ phân giải rộng
constexpr int CAMERA_HEIGHT = 720;      // Độ phân giải cao
constexpr int CAMERA_FPS = 60;          // Target FPS

// ===== TIMEOUTS =====
constexpr int CLIENT_STARTUP_DELAY_MS = 2000;  // Độ trễ khởi động server
```

**Sau khi chỉnh sửa**, rebuild server:
```powershell
cmake --build build --config Debug --target server
```

### Cấu Hình Flask Server

Chỉnh sửa [app.py](app.py):

```python
# ===== CÀI ĐẶT KẾT NỐI =====
VM_IP = "127.0.0.1"  # Địa chỉ IP C++ server

# Cấu hình port (phải khớp với C++ constants.h)
COMMAND_PORT = 8888
DATA_PORT = 8889
LIVESTREAM_PORT = 8890
KEYLOG_PORT = 8891

# ===== CÀI ĐẶT PROTOCOL =====
HEADER_FORMAT = '>HHIIIII'  # Big-endian, 7 trường
HEADER_SIZE = 24            # Bytes

# ===== CÀI ĐẶT SERVER =====
# Ở cuối file:
if __name__ == '__main__':
    app.run(
        host='0.0.0.0',    # Lắng nghe trên tất cả interfaces
        port=5001,         # Cổng Flask web server
        debug=True,        # Bật auto-reload khi code thay đổi
        threaded=True      # Xử lý nhiều requests đồng thời
    )
```

### Tối Ưu Hiệu Năng

#### Cho Hệ Thống Yếu (CPU < 4 cores, RAM < 8GB):
```python
# app.py - Giảm tần suất polling
dashboardInterval = setInterval(updateDashboardStats, 5000);  // 5s thay vì 3s

# Giảm FPS screen stream
fps = 5  // Trong hàm screen_stream_simple()
```

#### Cho Hệ Thống Mạnh (CPU > 8 cores, RAM > 16GB):
```cpp
// constants.h - Tăng chất lượng
constexpr int CAMERA_WIDTH = 1920;
constexpr int CAMERA_HEIGHT = 1080;
constexpr int CAMERA_FPS = 120;
constexpr int DATA_CHUNK_SIZE = 4 * 1024 * 1024;  // 4MB chunks
```

### Cấu Hình Firewall

**Windows Firewall (PowerShell Admin)**:
```powershell
# Cho phép C++ server
New-NetFirewallRule -DisplayName "NET-GHOST Server" `
    -Direction Inbound -Program "C:\python\Projects\Socket\SocketPro\build\Debug\server.exe" `
    -Action Allow

# Cho phép các cổng cụ thể
New-NetFirewallRule -DisplayName "NET-GHOST Ports" `
    -Direction Inbound -Protocol TCP -LocalPort 8888-8891 -Action Allow

# Cho phép Flask (nếu truy cập từ máy khác)
New-NetFirewallRule -DisplayName "Flask Web UI" `
    -Direction Inbound -Protocol TCP -LocalPort 5001 -Action Allow
```

---

## 📚 API Reference

### REST API Endpoints

#### System Monitoring

**GET `/api/ping`**
```json
// Response
{
  "status": "online" | "offline"
}
```

**GET `/api/system/stats`**
```json
{
  "cpu_percent": 45.2,
  "mem_used_gb": 8.3,
  "mem_total_gb": 16.0,
  "mem_percent": 51.9,
  "uptime": "5h 32m",
  "process_count": 234,
  "disk_used_gb": 250.5,
  "disk_total_gb": 500.0,
  "disk_percent": 50.1,
  "net_sent_mb": 1024.5,
  "net_recv_mb": 2048.3
}
```

**GET `/api/network/stats`**
```json
{
  "download_speed_mb": 2.5,
  "upload_speed_mb": 0.8,
  "total_sent_gb": 15.2,
  "total_recv_gb": 45.8,
  "packets_sent": 1500000,
  "packets_recv": 2000000,
  "connections": 42
}
```

#### Process Management

**GET `/api/processes`**
```json
[
  {
    "pid": "1234",
    "name": "chrome.exe",
    "memory": "512 MB",
    "cpu": "15.2%"
  },
  // ...more processes
]
```

**GET `/api/apps`**
```json
[
  {
    "name": "Google Chrome",
    "version": "120.0.6099.109",
    "publisher": "Google LLC",
    "path": "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe"
  },
  // ...more apps
]
```

**POST `/api/control`**
```json
// Request
{
  "cmd": "START",  // START | STOP | RESTART | SHUTDOWN
  "arg": "C:\\path\\to\\program.exe"  // Optional for most commands
}

// Response
{
  "response": "ACK: Process started successfully"
}
```

**POST `/api/kill`**
```json
// Kill by PID
{
  "type": "process",
  "target": "1234"
}

// Kill by app name
{
  "type": "app",
  "target": "chrome.exe"
}

// Response
{
  "status": "success" | "error",
  "message": "Process terminated successfully"
}
```

#### Screen Capture

**GET `/api/screenshot?save=true`**
- **Query Params**: 
  - `save` (tùy chọn): `true` (mặc định) | `false` - Lưu vào thư mục `screenshots/`
- **Response**: JPEG image binary
- **Headers**: `Content-Type: image/jpeg`

**GET `/screen_feed`**
- Giám sát màn hình real-time
- **Response**: MJPEG multipart stream
- **FPS**: ~10 frames mỗi giây

#### Webcam

**GET `/video_feed`**
- Live webcam streaming
- **Response**: MJPEG multipart stream
- **FPS**: 30-60 (cấu hình trong C++ server)

**POST `/api/webcam/record`**
```json
// Bắt đầu ghi hình
{
  "action": "start"
}

// Dừng ghi hình
{
  "action": "stop"
}

// Response
{
  "status": "Recording started - video will be saved to recordings folder"
}
```

**POST `/api/webcam/off`**
```json
// Response
{
  "status": "Stream stopped successfully"
}
```

#### Keylogger

**POST `/api/keylog/toggle`**
```json
// Bắt đầu keylogger
{
  "state": true
}

// Dừng keylogger
{
  "state": false
}

// Response
{
  "status": "ok"
}
```

**GET `/api/keylog/stream`**
- Server-Sent Events (SSE) stream
- **Response**: `text/event-stream`
- **Format**: 
  ```
  data: [2025-01-15 10:30:45] Hello World
  
  data: [2025-01-15 10:30:50] [ENTER]
  
  ```

**GET `/api/keylog/stats`**
```json
{
  "is_running": true,
  "total_keys": 1523,
  "session_keys": 245,
  "last_activity": "2s ago" | "5m ago" | "Never"
}
```

### C++ Server Commands

Gửi qua COMMAND_PORT (8888) dưới dạng chuỗi ASCII:

| Command | Parameters | Response Channel | Description |
|---------|-----------|------------------|-------------|
| `PING` | None | COMMAND_PORT | Health check, returns "PONG" |
| `PROCESS_LIST` | None | DATA_PORT | CSV list of processes |
| `APP_LIST` | None | DATA_PORT | CSV list of installed apps |
| `SYSTEM_STATS` | None | COMMAND_PORT | JSON system statistics |
| `SCREEN_CAPTURE` | None | DATA_PORT | JPEG screenshot |
| `START <path>` | Executable path | COMMAND_PORT | Launch process |
| `STOP <pid>` | Process ID | COMMAND_PORT | Kill process by PID |
| `APP_START <path>` | Application path | COMMAND_PORT | Start application |
| `APP_STOP <name>` | Process name | COMMAND_PORT | Stop application by name |
| `LIVESTREAM` | None | LIVESTREAM_PORT | Start webcam stream |
| `STOPLIVESTREAM` | None | COMMAND_PORT | Stop webcam stream |
| `KEYLOG` | None | KEYLOG_PORT | Start keylogger stream |
| `STOPKEYLOG` | None | COMMAND_PORT | Stop keylogger |
| `SHUTDOWN` | None | COMMAND_PORT | Shutdown system |
| `RESTART` | None | COMMAND_PORT | Restart system |

---

## 🔧 Phát Triển

### Quy Trình Phát Triển

```bash
# 1. Thay đổi code C++
# Chỉnh sửa files trong src/

# 2. Rebuild target cụ thể
cmake --build build --config Debug --target server

# 3. Test tích hợp Flask
python app.py

# 4. Debug với VS Code
# Launch configuration trong .vscode/launch.json
```

### Thêm Commands Mới

**Bước 1**: Thêm command constant trong C++ `constants.h`
```cpp
constexpr const char* CMD_YOUR_COMMAND = "YOUR_COMMAND";
```

**Bước 2**: Implement handler trong C++ server
```cpp
if (command == CMD_YOUR_COMMAND) {
    // Logic của bạn ở đây
    sendResponse(clientSocket, "ACK: Command executed");
}
```

**Bước 3**: Thêm Flask API endpoint trong `app.py`
```python
@app.route('/api/your-endpoint', methods=['POST'])
def your_endpoint():
    response = send_command_packet("YOUR_COMMAND")
    return jsonify({'status': response})
```

**Bước 4**: Cập nhật dashboard UI
```javascript
// Thêm button trong dashboard.html
<button onclick="fetch('/api/your-endpoint', {method: 'POST'})">
    Tính Năng Của Bạn
</button>
```

---

## 🐛 Khắc Phục Sự Cố

### Các Vấn Đề Thường Gặp

#### 1. "Connection Refused" khi Khởi Động Flask

**Triệu chứng:**
```
[ERROR] Socket connection failed: [WinError 10061] No connection could be made
```

**Giải pháp:**
- ✅ **Khởi động C++ server trước**: `.\build\Debug\server.exe`
- ✅ **Kiểm tra ports**: Xác minh 8888-8891 không bị chiếm dụng
  ```powershell
  netstat -ano | findstr "8888"
  ```
- ✅ **Firewall**: Cho phép server.exe qua Windows Firewall
- ✅ **Xác minh IP**: Đảm bảo `VM_IP` trong app.py khớp với IP server

#### 2. Không Tìm Thấy OpenCV DLL

**Triệu chứng:**
```
The code execution cannot proceed because opencv_world4110.dll was not found
```

**Giải pháp:**
- ✅ **Copy DLL**: 
  ```powershell
  copy C:\opencv\build\x64\vc16\bin\opencv_world4110.dll build\Debug\
  ```
- ✅ **Thêm vào PATH**: 
  ```powershell
  $env:PATH += ";C:\opencv\build\x64\vc16\bin"
  ```

#### 3. Flask Module Không Tìm Thấy

**Giải pháp:**
```bash
pip install -r requirements.txt
# Hoặc kích hoạt virtual environment trước:
venv\Scripts\activate
```

#### 4. Webcam Hiển Thị Màn Hình Đen

**Giải pháp:**
- ✅ **Quyền camera**: Windows Settings → Privacy → Camera
- ✅ **Camera đang dùng**: Đóng các app khác đang dùng webcam
- ✅ **Thử index khác**: Chỉnh sửa livestream.cpp `cap.open(1);`

#### 5. CPU Usage Cao

**Giải pháp:**
- ✅ **Giảm polling**: Đổi khoảng cập nhật dashboard thành 5000ms
- ✅ **Giảm FPS**: Giảm `CAMERA_FPS` trong constants.h
- ✅ **Tắt auto-refresh**: Comment out `setInterval()`

### Cải Tiến Khuyến Nghị

1. **Thêm Authentication**: Implement Flask-Login hoặc JWT
2. **Bật HTTPS**: Sử dụng SSL certificates
3. **Mã Hóa TCP**: TLS wrapper cho sockets
4. **Rate Limiting**: Ngăn chặn DoS attacks
5. **Input Validation**: Sanitize tất cả user inputs

---

## 📚 Tài Liệu Bổ Sung

- **[.github/copilot-instructions.md](.github/copilot-instructions.md)** - Hướng dẫn phát triển Flask
- **Architecture.md** - Kiến trúc C++ server
- **requirements.md** - Đặc tả kỹ thuật

### Tiêu Chuẩn Code

**C++**: Modern C++17, RAII, xử lý lỗi
**Python**: PEP 8, type hints, docstrings

---

### Công Nghệ

- **[Flask](https://flask.palletsprojects.com/)** - Web framework
- **[OpenCV](https://opencv.org/)** - Computer vision
- **[psutil](https://github.com/giampaolo/psutil)** - System monitoring
- **[Tailwind CSS](https://tailwindcss.com/)** - UI framework
- **[Font Awesome](https://fontawesome.com/)** - Icons

### Công Cụ

- **Visual Studio 2022** - C++ compiler
- **CMake** - Build system
- **VS Code** - Code editor

---

**© 2025 NET-GHOST Project | Chỉ Sử Dụng Cho Giáo Dục**

Cập Nhật Lần Cuối: Tháng 12 2025 | Trạng Thái: ✅ Sẵn Sàng

</div>
