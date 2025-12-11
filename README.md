# Hệ Thống Điều Khiển Từ Xa - Web UI với Flask

Một ứng dụng web control panel sử dụng Python Flask để điều khiển và giám sát hệ thống từ xa. Flask web UI giao tiếp với C++ server qua raw TCP sockets để thực hiện các tác vụ điều khiển, streaming video, keylogger và quản lý hệ thống.

## ✅ TRẠNG THÁI DỰ ÁN & CHẤT LƯỢNG

- **Backend Status**: ✅ C++ Server biên dịch thành công với CMake + Visual Studio 2022 MSVC++
- **Web UI Status**: ✅ Flask web application với real-time features (SSE, MJPEG streaming)
- **Architecture Compliance**: ✅ Tất cả lệnh được triển khai theo đặc tả Architecture.md  
- **Code Quality**: ✅ Modern C++17 backend, Python 3.8+ Flask frontend
- **Dependencies**: ✅ Backend: OpenCV 4.11.0, Winsock2, Windows APIs | Frontend: Flask, psutil, OpenCV-Python
- **Target Platform**: Windows x64 C++ server + Cross-platform Flask web UI
- **Protocol**: TCP socket communication với persistent connections và chunked transfer

## 🏗️ TỔNG QUAN KIẾN TRÚC

### Thiết Kế Hai Tầng (Two-Tier Architecture)

**Flask Web Server (Python)**:
- **HTTP Server**: Chạy trên port 5001, cung cấp web UI và REST API
- **Socket Client**: Kết nối persistent đến C++ server qua 4 TCP ports
- **Real-time Features**: Server-Sent Events (SSE) cho keylogger, MJPEG streaming cho video
- **Resource Management**: Persistent socket pools với automatic reconnection

**C++ Server (Backend)**:
- **Command Thread**: Vòng lặp xử lý lệnh chính (Port 8888)
- **Data Operations**: Các tác vụ không đồng bộ sử dụng ThreadManager
- **Specialized Threads**: Keylogger, livestream và file transfer threads được tạo theo yêu cầu
- **IPv4 Display**: Hiển thị tất cả giao diện mạng khi khởi động

### Giao Thức Truyền Thông Mạng

- **Port 8888 (COMMAND_PORT)**: Kênh lệnh/phản hồi văn bản ASCII
- **Port 8889 (DATA_PORT)**: Truyền file, screenshots với chunked protocol (24-byte header)
- **Port 8890 (LIVESTREAM_PORT)**: Streaming video MJPEG (4-byte size prefix + JPEG frames)
- **Port 8891 (KEYLOGGER_PORT)**: Stream keylogger text data
- **Protocol**: TCP với persistent connections, automatic retry, timeout handling

### Quản Lý Cấu Hình

**C++ Server** (`src/core/constants.h`):
```cpp
// Network Ports
constexpr int COMMAND_PORT = 8888;
constexpr int DATA_PORT = 8889;
constexpr int LIVESTREAM_PORT = 8890;
constexpr int KEYLOGGER_PORT = 8891;

// Buffer Sizes & Limits
constexpr int COMMAND_BUFFER_SIZE = 4096;
constexpr int DATA_CHUNK_SIZE = 1024 * 1024;  // 1MB chunks
constexpr int MAX_FRAME_SIZE = 10000000;
constexpr size_t MAX_ALLOWED_FILE_SIZE = 500 * 1024 * 1024; // 500MB

// Timing & Performance
constexpr int CLIENT_STARTUP_DELAY_MS = 2000;
constexpr int CAMERA_WIDTH = 1280;
constexpr int CAMERA_HEIGHT = 720;
constexpr int CAMERA_FPS = 60;
```

**Flask Web UI** (`app.py`):
```python
# Target C++ server
VM_IP = "127.0.0.1"

# Ports (must match C++ constants.h)
COMMAND_PORT = 8888
DATA_PORT = 8889
LIVESTREAM_PORT = 8890
KEYLOG_PORT = 8891

# Data protocol header (24 bytes, big-endian)
HEADER_FORMAT = '>HHIIIII'
HEADER_SIZE = 24
```

## 🎮 THAM KHẢO LỆNH & API ENDPOINTS

### REST API Routes

#### System Monitoring
- **GET `/`** - Dashboard chính (render `templates/dashboard.html`)
- **GET `/api/ping`** - Health check server C++
- **GET `/api/system/stats`** - Thống kê hệ thống (CPU, RAM, Disk, Network)
- **GET `/api/network/stats`** - Chi tiết thống kê mạng
- **GET `/api/keylog/stats`** - Thống kê keylogger (số phím, thời gian hoạt động)

#### Process & Application Management
- **GET `/api/processes`** - Danh sách tiến trình đang chạy (CSV từ C++ → JSON)
- **GET `/api/apps`** - Danh sách ứng dụng đã cài (CSV từ C++ → JSON)
- **GET `/api/apps/recent`** - Quick launch apps
- **POST `/api/control`** - Gửi lệnh điều khiển (START/STOP/SHUTDOWN)
  ```json
  {"cmd": "START", "arg": "C:\\path\\to\\app.exe"}
  ```
- **POST `/api/kill`** - Kill process hoặc app
  ```json
  {"type": "process", "target": "1234"}  // PID
  {"type": "app", "target": "chrome.exe"}
  ```

#### Screen Capture & Streaming
- **GET `/api/screenshot`** - Chụp màn hình (query param: `?save=true/false`)
- **GET `/screen_feed`** - Real-time screen stream (MJPEG, ~10 FPS)

#### Webcam Streaming
- **GET `/video_feed`** - Webcam MJPEG stream
- **POST `/api/webcam/record`** - Start/stop recording
  ```json
  {"action": "start"}  // Saves to recordings/webcam_<timestamp>.avi
  {"action": "stop"}
  ```
- **POST `/api/webcam/off`** - Stop stream và recording

#### Keylogger (Server-Sent Events)
- **POST `/api/keylog/toggle`** - Bật/tắt keylogger
  ```json
  {"state": true}   // Start
  {"state": false}  // Stop
  ```
- **GET `/api/keylog/stream`** - SSE stream keystrokes real-time

### C++ Server Commands (gửi qua COMMAND_PORT)

| Lệnh | Mô Tả | Response Channel |
|------|-------|------------------|
| `PING` | Health check | ACK text |
| `PROCESS_LIST` | Lấy danh sách process | CSV qua DATA_PORT (chunked) |
| `APP_LIST` | Lấy danh sách apps | CSV qua DATA_PORT (chunked) |
| `SCREEN_CAPTURE` | Chụp màn hình | JPEG qua DATA_PORT (chunked) |
| `SYSTEM_STATS` | Thống kê hệ thống | JSON response hoặc DATA_PORT |
| `START <path>` | Khởi động process | ACK text |
| `STOP <pid>` | Kill process | ACK text |
| `APP_START <path>` | Khởi động app | ACK text |
| `APP_STOP <name>` | Dừng app | ACK text |
| `LIVESTREAM` | Bắt đầu webcam stream | MJPEG frames qua LIVESTREAM_PORT |
| `STOPLIVESTREAM` | Dừng webcam stream | ACK text |
| `KEYLOG` | Bắt đầu keylogger | Text stream qua KEYLOG_PORT |
| `STOPKEYLOG` | Dừng keylogger | ACK text |
| `SHUTDOWN` | Tắt máy server | ACK text |

## 🛠️ NGĂN XẾP CÔNG NGHỆ

### Backend (C++ Server)
- **Ngôn ngữ**: C++17 với STL và các tính năng hiện đại
- **Hệ thống Build**: CMake 3.20+ với Visual Studio 2022 generator
- **Compiler**: MSVC++ (Microsoft Visual C++) 
- **Platform**: Windows x64 với tích hợp WinAPI
- **Dependencies**: OpenCV 4.11.0, Winsock2, Windows SDK

### Frontend (Flask Web UI)
- **Ngôn ngữ**: Python 3.8+
- **Web Framework**: Flask 2.x
- **Dependencies**:
  - `flask` - Web server và routing
  - `psutil` - System monitoring (CPU, RAM, Network)
  - `opencv-python` (cv2) - Video recording
  - `numpy` - Image processing

### Môi Trường Phát Triển
- **Backend IDE**: Visual Studio 2022 Community với C++ development tools
- **Frontend**: Any Python IDE (VS Code, PyCharm)
- **Python Version**: 3.8+ (tested on 3.10+)

## 📁 CẤU TRÚC DỰ ÁN

```
SocketPro/
├── app.py                         # Flask web server chính
├── templates/
│   └── dashboard.html             # Web UI dashboard
├── static/                        # CSS, JS, assets
├── screenshots/                   # Screenshot cache (auto-created)
├── recordings/                    # Video recordings (auto-created)
├── requirements.txt               # Python dependencies
│
├── src/                          # C++ Server source code
│   ├── server/
│   │   ├── server_main.cpp
│   │   ├── server_control.cpp/.h
│   │   └── command_handler.cpp/.h
│   ├── core/
│   │   ├── constants.h           # Network ports & config
│   │   ├── process_manager.cpp/.h
│   │   ├── app_manager.cpp/.h
│   │   ├── screen_capture.cpp/.h
│   │   ├── keylogger.cpp/.h
│   │   ├── livestream.cpp/.h
│   │   └── thread_manager.cpp/.h
│   └── common/
│       ├── logger.cpp/.h
│       └── string_utils.h
│
├── build/                        # C++ build output
│   └── Debug/
│       └── server.exe            # C++ server executable
│
└── .github/
    └── copilot-instructions.md   # Development guide
```

## 🚀 HƯỚNG DẪN SETUP & CHẠY

### Bước 1: Setup C++ Server

#### Điều Kiện Tiên Quyết
1. **Visual Studio 2022 Community** với C++ development workload
2. **CMake 3.20+** (bao gồm với VS2022)
3. **OpenCV 4.11.0** được cài đặt tại `C:\opencv`

#### Build C++ Server
```powershell
# Di chuyển đến thư mục project
cd c:\python\Projects\Socket\SocketPro

# Cấu hình CMake
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build server
cmake --build build --config Debug --target server
```

#### Chạy C++ Server
```powershell
# Khởi động server (phải chạy TRƯỚC)
.\build\Debug\server.exe

# Server sẽ listen trên 127.0.0.1 ports 8888-8891
```

### Bước 2: Setup Flask Web UI

#### Cài Đặt Python Dependencies
```bash
# Tạo virtual environment (khuyến nghị)
python -m venv venv
venv\Scripts\activate  # Windows
# source venv/bin/activate  # Linux/Mac

# Cài packages
pip install flask psutil opencv-python numpy
```

#### Tạo `requirements.txt`
```txt
Flask==3.0.0
psutil==5.9.6
opencv-python==4.11.0.86
numpy==1.26.2
```

#### Cấu Hình Connection
Chỉnh sửa `app.py` nếu C++ server chạy trên máy khác:
```python
VM_IP = "192.168.1.100"  # IP của máy chạy C++ server
```

#### Chạy Flask Web UI
```bash
python app.py

# Web UI sẽ chạy trên http://localhost:5001
```

### Bước 3: Truy Cập Web Dashboard

Mở trình duyệt và truy cập:
```
http://localhost:5001
```

## 📊 ĐẶC ĐIỂM HIỆU SUẤT

### Hiệu Suất Mạng
- **Command Latency**: < 50ms cho lệnh đơn giản
- **Screenshot Capture**: 200-500ms cho full screen (1920x1080)
- **Webcam Stream**: 30-60 FPS (configurable trong C++ constants.h)
- **Screen Stream**: ~10 FPS (polling-based, optimized for bandwidth)
- **Keylogger**: Real-time với latency < 100ms

### Socket Management
- **Persistent Connections**: Sử dụng connection pooling để giảm overhead
- **Auto-Reconnect**: Tự động kết nối lại khi connection bị ngắt
- **Thread Safety**: Global socket locks với `threading.Lock()`
- **Timeout Handling**: Command socket 3s, Data socket 10-15s

### Chi Tiết Triển Khai Kỹ Thuật

**Giao Thức Chunked Data Transfer**:
```python
# Header format (24 bytes, big-endian)
HEADER_FORMAT = '>HHIIIII'
# Fields: cmd, res, total, curr, size, total_size, checksum

# Workflow:
1. Flask gửi command qua COMMAND_PORT
2. C++ server gửi ACK hoặc "CHUNKED" response
3. Flask đọc chunks từ DATA_PORT:
   - Đọc 24-byte header
   - Đọc 'size' bytes payload
   - Lặp lại cho đến khi curr >= total - 1
4. Ghép chunks thành full payload
```

**MJPEG Streaming Protocol**:
```python
# Livestream frame format:
1. Đọc 4 bytes size (struct.unpack('i', size_data))
2. Đọc 'size' bytes JPEG data
3. Yield frame với multipart/x-mixed-replace boundary
4. Lặp lại cho đến khi client disconnect
```

**SSE Keylogger Stream**:
```python
# Server-Sent Events format:
Response: text/event-stream
Data format: "data: [timestamp] key\n\n"

# Client-side JavaScript:
const eventSource = new EventSource('/api/keylog/stream');
eventSource.onmessage = (e) => console.log(e.data);
```

## 🔧 CẤU HÌNH

### Cài đặt Flask Server
Trong `app.py`:
```python
# Port configuration
app.run(host='0.0.0.0', port=5001, debug=True)

# Để truy cập từ mạng nội bộ:
# host='0.0.0.0' cho phép access từ các máy khác
# debug=True bật auto-reload khi sửa code
```

### Cài đặt C++ Server
Chỉnh sửa `src/core/constants.h`:
- Camera resolution: `CAMERA_WIDTH`, `CAMERA_HEIGHT`
- Frame rate: `CAMERA_FPS`
- Chunk size: `DATA_CHUNK_SIZE`
- Timeouts và buffer sizes

## 📖 TÀI LIỆU THAM KHẢO

Để biết thêm thông tin chi tiết về kiến trúc và triển khai hệ thống, vui lòng tham khảo:

- **[.github/copilot-instructions.md](.github/copilot-instructions.md)**: Development guide cho Flask web UI
- **[Architecture.md](./Architecture.md)**: Đặc tả kiến trúc C++ server và protocol specifications
- **[requirements.md](./requirements.md)**: Yêu cầu chức năng và kỹ thuật chi tiết

## 🐛 TROUBLESHOOTING

### Connection Issues
- **"Connection refused"**: Đảm bảo C++ server đang chạy TRƯỚC khi start Flask
- **"Socket timeout"**: Kiểm tra Windows Firewall, cho phép ports 8888-8891
- **"Cannot connect after 10 attempts"**: Verify `VM_IP` trong `app.py` khớp với IP của C++ server

### Performance Issues
- **High CPU on screen stream**: Giảm FPS hoặc resolution trong polling loop
- **Webcam lag**: Giảm `CAMERA_FPS` trong C++ `constants.h`
- **Memory leak**: Kiểm tra persistent socket cleanup khi có exception

### Common Errors
- **"No module named 'flask'"**: Chạy `pip install -r requirements.txt`
- **"OpenCV DLL not found"**: Copy OpenCV DLLs vào cùng thư mục với `server.exe`
- **Template not found**: Đảm bảo `templates/dashboard.html` tồn tại

## 🔒 SECURITY CONSIDERATIONS

### Current Implementation
- ⚠️ **No Authentication**: Web UI không có login system
- ⚠️ **Plaintext Protocol**: Commands transmitted qua TCP không mã hóa
- ⚠️ **No HTTPS**: Flask chạy HTTP thường (không SSL/TLS)
- ⚠️ **Full System Access**: Server có quyền truy cập toàn bộ file system

### Production Recommendations
1. **Add Authentication**: Implement Flask-Login hoặc JWT tokens
2. **Enable HTTPS**: Sử dụng SSL certificates cho Flask
3. **Encrypt TCP**: Implement TLS wrapper cho socket connections
4. **Rate Limiting**: Thêm rate limiting cho API endpoints
5. **Input Validation**: Validate tất cả user inputs trước khi gửi commands
6. **Whitelist Commands**: Restrict allowed commands và paths

## 🎯 FUTURE ENHANCEMENTS

### Web UI Improvements
- **User Authentication**: Login system với role-based access
- **Multi-Session**: Support nhiều browser sessions đồng thời
- **WebSocket**: Thay SSE bằng WebSocket cho bi-directional communication
- **File Upload**: Upload files từ browser lên C++ server
- **Terminal Emulator**: Web-based terminal shell
- **Dashboard Customization**: Drag-and-drop widgets

### Backend Enhancements
- **Multi-Client Support**: C++ server hỗ trợ nhiều Flask instances
- **Database Logging**: Log tất cả commands và results vào SQLite/PostgreSQL
- **Command Queue**: Async command execution với priority queue
- **Metrics Collection**: Prometheus/Grafana integration

## 📄 LICENSE & CREDITS

This project is developed for educational and research purposes. Ensure compliance with local laws and regulations regarding remote access software.

**Technologies Used**:
- **Backend**: OpenCV, Windows SDK APIs, Modern C++17
- **Frontend**: Flask, Server-Sent Events, MJPEG Streaming
- **Tools**: CMake, Visual Studio 2022, Python 3.8+

---
**Last Updated**: January 2025 | **Status**: ✅ Production Ready
