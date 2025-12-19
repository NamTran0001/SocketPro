# 🕵️ NET-GHOST v2.0 - Remote System Control & Monitoring

<div align="center">

![Status](https://img.shields.io/badge/status-production%20ready-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue)
![Python](https://img.shields.io/badge/python-3.8+-blue)
![C++](https://img.shields.io/badge/C++-17-blue)
![License](https://img.shields.io/badge/license-Educational-orange)

**Professional remote system management platform with real-time monitoring and control capabilities**

[Features](#-features) • [Quick Start](#-quick-start) • [Documentation](#-documentation) • [API Reference](#-api-reference) • [Architecture](#-architecture)

</div>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Architecture](#-architecture)
- [Requirements](#-requirements)
- [Quick Start](#-quick-start)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [API Reference](#-api-reference)
- [Development](#-development)
- [Troubleshooting](#-troubleshooting)
- [Security](#-security)
- [Contributing](#-contributing)

---

## 🌟 Overview

NET-GHOST is a powerful **two-tier remote administration system** combining a Python Flask web interface with a high-performance C++ backend server. It provides comprehensive system control, real-time monitoring, and media streaming capabilities through an elegant web dashboard.

### 🎯 Project Status

| Component | Status | Details |
|-----------|--------|---------|
| **C++ Backend** | ✅ Production Ready | Built with CMake + VS2022 MSVC++, C++17 |
| **Flask Web UI** | ✅ Production Ready | Modern responsive dashboard with Tailwind CSS |
| **Real-time Features** | ✅ Fully Functional | SSE keylogger, MJPEG streaming (webcam/screen) |
| **Architecture** | ✅ Compliant | Follows Architecture.md specification |
| **Network Protocol** | ✅ Stable | TCP persistent connections with chunked transfer |
| **Dependencies** | ✅ Resolved | OpenCV 4.11.0, Flask, psutil, Windows SDK |

### 🔑 Key Highlights

- 🌐 **Modern Web Interface** - Clean dashboard with real-time updates every 3 seconds
- 🚀 **High Performance** - Persistent TCP connections, multi-threaded C++ backend
- 📹 **Media Streaming** - Webcam & screen monitoring with MJPEG protocol (30-60 FPS)
- ⌨️ **Live Keylogger** - Real-time keystroke streaming via Server-Sent Events (SSE)
- 💻 **System Control** - Process management, application launching, power control
- 📊 **Resource Monitoring** - CPU, RAM, disk, network stats with visual charts
- 🔌 **Protocol Flexibility** - 4 specialized TCP ports for different operations

---

## ✨ Features

### 🖥️ System Management
- **Process Monitor** - View all running processes with PID, name, memory usage, CPU usage
- **Kill Processes** - Terminate processes by PID or application name
- **Application Control** - Start/stop installed applications remotely
- **Power Management** - Shutdown, restart system commands
- **System Stats** - Real-time CPU, RAM, disk usage, network I/O monitoring

### 📸 Media Capture & Streaming
- **Screenshot Capture** - On-demand full-screen screenshots (JPEG, 1920x1080)
- **Live Screen Monitoring** - Real-time screen streaming (~10 FPS, MJPEG)
- **Webcam Streaming** - HD video streaming (configurable 30-60 FPS)
- **Video Recording** - Record webcam streams to AVI files with timestamps
- **Auto-save Screenshots** - Automatic screenshot archiving to `screenshots/` folder

### ⌨️ Keylogging & Monitoring
- **Real-time Keylogger** - Live keystroke capture via Server-Sent Events
- **Session Statistics** - Track total keys pressed, session duration, last activity
- **Terminal Display** - Console-style keylog viewer with auto-scroll
- **Start/Stop Control** - Toggle keylogger from web interface

### 📊 Dashboard & Analytics
- **Resource Graphs** - Visual CPU/RAM usage indicators
- **Network Stats** - Upload/download speeds, total data transferred
- **Recent Apps** - Quick launch shortcuts for common applications
- **Uptime Tracking** - System boot time and uptime duration
- **Connection Status** - Real-time server health monitoring

### 🔧 Advanced Features
- **Persistent Connections** - Socket connection pooling for low latency
- **Auto-reconnect** - Automatic retry on connection failures
- **Multi-tab Interface** - Organized sidebar navigation (Dashboard, Processes, Apps, Screen, Webcam, Keylog)
- **Responsive Design** - Mobile-friendly interface with Tailwind CSS
- **Dark Theme** - Professional cyberpunk-inspired UI with cyan accents

---

## 🏗️ Architecture

---

## 🏗️ Architecture

### System Design

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
│  │  • Process ASCII commands                                 │   │
│  │  • Route to appropriate handlers                          │   │
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
│  │  • Async data operations                                  │   │
│  │  • Chunked file transfer                                  │   │
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

### Network Protocol Specification

| Port | Channel | Protocol | Purpose | Data Format |
|------|---------|----------|---------|-------------|
| **8888** | COMMAND | TCP Text | Send commands, receive ACK | ASCII strings |
| **8889** | DATA | TCP Binary | Large payloads (CSV, images) | 24-byte header + chunks |
| **8890** | LIVESTREAM | TCP Binary | Video streaming | 4-byte size + JPEG frames |
| **8891** | KEYLOGGER | TCP Text | Keystroke streaming | Text lines with timestamps |

#### Data Channel Protocol (Port 8889)

**Header Format** (24 bytes, big-endian):
```python
HEADER_FORMAT = '>HHIIIII'
# Fields:
# - cmd (uint16)        : Command code
# - res (uint16)        : Reserved
# - total (uint32)      : Total number of chunks
# - curr (uint32)       : Current chunk index (0-based)
# - size (uint32)       : Size of current chunk payload (bytes)
# - total_size (uint32) : Total payload size across all chunks
# - checksum (uint32)   : CRC32 checksum (optional)
```

**Transfer Flow**:
1. Flask sends command via COMMAND_PORT (e.g., `"SCREEN_CAPTURE"`)
2. C++ server acknowledges on COMMAND_PORT
3. C++ server sends data chunks via DATA_PORT:
   - Each chunk: [24-byte header][payload bytes]
   - Loop until `curr >= total - 1`
4. Flask reassembles chunks into complete payload

#### Livestream Protocol (Port 8890)

**Frame Format**:
```
[4 bytes: int32 frame_size][frame_size bytes: JPEG data]
[4 bytes: int32 frame_size][frame_size bytes: JPEG data]
...
```

MJPEG streaming with boundary-based HTTP multipart response:
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

## 🛠️ Requirements

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

---

## 🛠️ Requirements

### System Requirements

| Component | Requirement | Notes |
|-----------|-------------|-------|
| **Operating System** | Windows 10/11 x64 | C++ server requires Windows APIs |
| **Python** | 3.8 or higher | Tested on Python 3.10+ |
| **Compiler** | MSVC++ (VS 2022) | For building C++ server |
| **CMake** | 3.20 or higher | Build system |
| **RAM** | 4GB minimum, 8GB recommended | For smooth streaming |
| **Network** | Local or LAN connection | TCP ports 8888-8891 must be accessible |

### Software Dependencies

#### C++ Server (Backend)
- **Visual Studio 2022 Community** (or Professional/Enterprise)
  - Workload: "Desktop development with C++"
  - Components: MSVC v143, Windows SDK, CMake tools
- **OpenCV 4.11.0**
  - Download: [opencv.org](https://opencv.org/releases/)
  - Install path: `C:\opencv` (or configure CMake accordingly)
  - Required modules: core, imgcodecs, videoio, imgproc

#### Flask Web UI (Frontend)
```bash
pip install flask>=3.0.0
pip install psutil>=5.9.6
pip install opencv-python>=4.11.0
pip install numpy>=1.26.2
```

### Network Configuration
- **Firewall**: Allow inbound connections on ports 8888-8891
- **Router**: Port forwarding if accessing from external network (not recommended for security reasons)
- **Antivirus**: May need to whitelist `server.exe` and Python scripts

---

## 🚀 Quick Start

### 1️⃣ Start C++ Server (Backend)

```powershell
# Navigate to project directory
cd c:\python\Projects\Socket\SocketPro

# Build server (if not already built)
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target server

# Run server
.\build\Debug\server.exe
```

**Expected output:**
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

### 2️⃣ Start Flask Web UI

```bash
# Activate virtual environment (if using)
venv\Scripts\activate

# Run Flask application
python app.py
```

**Expected output:**
```
 * Serving Flask app 'app'
 * Debug mode: on
 * Running on http://0.0.0.0:5001 (Press CTRL+C to quit)
[DEBUG] Creating persistent socket connections...
[DEBUG] Command socket connected
[DEBUG] Data socket connected
```

### 3️⃣ Access Dashboard

Open browser and navigate to:
```
http://localhost:5001
```

🎉 **You should see the NET-GHOST dashboard with system statistics!**

---

## 📦 Installation

### Detailed Setup Guide

#### Step 1: Clone Repository

```bash
git clone https://github.com/yourusername/SocketPro.git
cd SocketPro
```

#### Step 2: Install C++ Dependencies

1. **Install Visual Studio 2022 Community**
   - Download: [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
   - During installation, select: "Desktop development with C++"
   - Ensure CMake tools are included

2. **Install OpenCV 4.11.0**
   ```powershell
   # Download OpenCV from opencv.org
   # Extract to C:\opencv\
   
   # Add to PATH (PowerShell Admin)
   $env:PATH += ";C:\opencv\build\x64\vc16\bin"
   
   # Verify installation
   dir C:\opencv\build\x64\vc16\bin\opencv_world4110.dll
   ```

3. **Configure OpenCV for CMake**
   ```powershell
   # Set environment variable
   [System.Environment]::SetEnvironmentVariable(
       "OpenCV_DIR",
       "C:\opencv\build",
       [System.EnvironmentVariableTarget]::User
   )
   ```

#### Step 3: Build C++ Server

```powershell
# Configure CMake project
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DOpenCV_DIR=C:\opencv\build

# Build all targets
cmake --build build --config Debug

# Or build specific target
cmake --build build --config Debug --target server

# Verify build
dir build\Debug\server.exe
```

#### Step 4: Setup Python Environment

```bash
# Create virtual environment (recommended)
python -m venv venv

# Activate environment
# Windows:
venv\Scripts\activate
# Linux/Mac:
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Or install manually:
pip install flask psutil opencv-python numpy
```

#### Step 5: Configure Connection

Edit [app.py](app.py) to set server IP:

```python
# Line 15: Set IP address of C++ server machine
VM_IP = "127.0.0.1"  # Local machine
# VM_IP = "192.168.1.100"  # Remote machine on LAN
```

#### Step 6: Create Required Directories

```powershell
# Flask will auto-create these, but you can prepare them:
New-Item -ItemType Directory -Force -Path screenshots
New-Item -ItemType Directory -Force -Path recordings
New-Item -ItemType Directory -Force -Path templates
```

#### Step 7: Run Application

```powershell
# Terminal 1: Start C++ server
.\build\Debug\server.exe

# Terminal 2: Start Flask UI
python app.py
```

---

## ⚙️ Configuration

---

## ⚙️ Configuration

### C++ Server Configuration

Edit [src/core/constants.h](src/core/constants.h):

```cpp
// ===== NETWORK CONFIGURATION =====
constexpr int COMMAND_PORT = 8888;      // Command channel
constexpr int DATA_PORT = 8889;         // Large data transfers
constexpr int LIVESTREAM_PORT = 8890;   // Video streaming
constexpr int KEYLOGGER_PORT = 8891;    // Keylogger stream

// ===== BUFFER & CHUNK SIZES =====
constexpr int COMMAND_BUFFER_SIZE = 4096;          // 4KB
constexpr int DATA_CHUNK_SIZE = 1024 * 1024;       // 1MB per chunk
constexpr int MAX_FRAME_SIZE = 10000000;           // 10MB max frame
constexpr size_t MAX_ALLOWED_FILE_SIZE = 500 * 1024 * 1024; // 500MB

// ===== CAMERA SETTINGS =====
constexpr int CAMERA_WIDTH = 1280;      // Resolution width
constexpr int CAMERA_HEIGHT = 720;      // Resolution height  
constexpr int CAMERA_FPS = 60;          // Target FPS

// ===== TIMEOUTS =====
constexpr int CLIENT_STARTUP_DELAY_MS = 2000;  // Server startup delay
```

**After modifying**, rebuild server:
```powershell
cmake --build build --config Debug --target server
```

### Flask Server Configuration

Edit [app.py](app.py):

```python
# ===== CONNECTION SETTINGS =====
VM_IP = "127.0.0.1"  # C++ server IP address

# Port configuration (must match C++ constants.h)
COMMAND_PORT = 8888
DATA_PORT = 8889
LIVESTREAM_PORT = 8890
KEYLOG_PORT = 8891

# ===== PROTOCOL SETTINGS =====
HEADER_FORMAT = '>HHIIIII'  # Big-endian, 7 fields
HEADER_SIZE = 24            # Bytes

# ===== SERVER SETTINGS =====
# At bottom of file:
if __name__ == '__main__':
    app.run(
        host='0.0.0.0',    # Listen on all interfaces
        port=5001,         # Flask web server port
        debug=True,        # Enable auto-reload on code changes
        threaded=True      # Handle multiple requests concurrently
    )
```

### Performance Tuning

#### For Low-End Systems (CPU < 4 cores, RAM < 8GB):
```python
# app.py - Reduce polling frequency
dashboardInterval = setInterval(updateDashboardStats, 5000);  # 5s instead of 3s

# Reduce screen stream FPS
fps = 5  # In screen_stream_simple() function
```

#### For High-End Systems (CPU > 8 cores, RAM > 16GB):
```cpp
// constants.h - Increase quality
constexpr int CAMERA_WIDTH = 1920;
constexpr int CAMERA_HEIGHT = 1080;
constexpr int CAMERA_FPS = 120;
constexpr int DATA_CHUNK_SIZE = 4 * 1024 * 1024;  // 4MB chunks
```

### Firewall Configuration

**Windows Firewall (PowerShell Admin)**:
```powershell
# Allow C++ server
New-NetFirewallRule -DisplayName "NET-GHOST Server" `
    -Direction Inbound -Program "C:\python\Projects\Socket\SocketPro\build\Debug\server.exe" `
    -Action Allow

# Allow specific ports
New-NetFirewallRule -DisplayName "NET-GHOST Ports" `
    -Direction Inbound -Protocol TCP -LocalPort 8888-8891 -Action Allow

# Allow Flask (if accessing from other machines)
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
  - `save` (optional): `true` (default) | `false` - Save to `screenshots/` folder
- **Response**: JPEG image binary
- **Headers**: `Content-Type: image/jpeg`

**GET `/screen_feed`**
- Real-time screen streaming
- **Response**: MJPEG multipart stream
- **FPS**: ~10 frames per second

#### Webcam

**GET `/video_feed`**
- Live webcam streaming
- **Response**: MJPEG multipart stream
- **FPS**: 30-60 (configured in C++ server)

**POST `/api/webcam/record`**
```json
// Start recording
{
  "action": "start"
}

// Stop recording
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
// Start keylogger
{
  "state": true
}

// Stop keylogger
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

Send via COMMAND_PORT (8888) as ASCII strings:

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

## 🔧 Development

### Project Structure

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
**Last Updated**: December 2025 | **Status**: ✅ Production Ready
