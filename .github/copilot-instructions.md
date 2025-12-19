## Quick context

This repository contains a small Python Flask web UI that communicates with a companion C++ server over raw TCP sockets. The Python side acts as a control panel and media bridge (screenshots, process lists, webcam MJPEG, keylogger SSE) for a Windows VM. The Flask app is the single web-facing service; the C++ server holds device/OS-specific logic and serves binary payloads on distinct ports.

## Big-picture architecture

- Flask web server (file: `app.py`) — HTTP UI + API endpoints.
- C++ server (separate repo/binary) — listens on multiple TCP ports. Python connects as a client to:
  - COMMAND_PORT (8888) — short textual commands + optional ACK responses
  - DATA_PORT (8889) — large binary payloads sent with a fixed header
  - LIVESTREAM_PORT (8890) — MJPEG frames with a 4-byte size prefix per JPEG
  - KEYLOG_PORT (8891) — streaming text (SSE events forwarded to browser)

Why this split: the C++ side performs privileged / platform-specific capture and control; Python provides a developer-friendly web UI and HTTP API surface.

## Key files & symbols (what to read first)

- `app.py` — main Flask app. Implements: `send_command_packet`, `receive_large_data`, `webcam_stream`, `screen_stream_simple`, SSE keylog streaming endpoints, and API routes like `/api/list/<type>`, `/api/screenshot`, `/video_feed`, `/screen_feed`, `/stream/keylog`.
- `templates/dashboard.html` — referenced by `index()` (not included in current snapshot). The app expects a `templates` directory for Flask rendering.
- Constants to keep in sync with C++: `VM_IP`, `COMMAND_PORT`, `DATA_PORT`, `LIVESTREAM_PORT`, `KEYLOG_PORT`, and `HEADER_FORMAT` / `HEADER_SIZE`.

## Protocol / data contracts (explicit)

- Header for large DATA channel (24 bytes): `HEADER_FORMAT = '>HHIIIII'` (Big-endian network byte order, packed as uint16, uint16, uint32, uint32, uint32, uint32, uint32). `HEADER_SIZE = 24`.
  - Fields unpacked as: `cmd, res, total, curr, size, total_size, checksum` in `receive_large_data()`.
  - Python expects chunks: header then `size` bytes of payload (loop until `curr >= total - 1`).
  - **IMPORTANT**: Changed from little-endian (`=`) to big-endian (`>`) to match C++ network byte order.

- Livestream protocol: server sends 4-byte signed int (C `int`) size then JPEG bytes. Python unpacks with `struct.unpack('i', size_data)`.

- Commands sent over COMMAND_PORT are plain ASCII strings, examples used by the app:
  - `PING` — health check
  - `PROCESS_LIST`, `APP_LIST` — triggers server to send CSV to DATA_PORT
  - `SCREEN_CAPTURE` — server sends a JPEG payload over DATA_PORT
  - `LIVESTREAM` / `STOPLIVESTREAM` — control webcam livestream
  - `SCREENSTREAM` / `STOPSCREENSTREAM` — control screen streaming (if supported)
  - `KEYLOG` / `STOPKEYLOG` — start/stop keylogger stream
  - `SYSTEM_STATS` — retrieve system statistics (CPU, RAM, disk, network)
  - `START <target>`, `STOP <target>`, `RESTART <target>` — control action format in `/api/control`
  - `APP_STOP <name.exe>` — kill process by name

## Socket connection management

- **Persistent connections**: The app uses persistent socket connections (`_cmd_socket`, `_data_socket`) that are kept alive across requests to avoid reconnection overhead.
- Thread-safe: Uses `_socket_lock` and `_cmd_lock` to prevent race conditions.
- Auto-reconnection: `get_persistent_sockets()` validates connections and recreates them if broken.
- Timeouts: Command socket (3s), Data socket (10-15s depending on operation).

## Developer workflows & run/debug tips

- Run locally: make sure Python 3.8+ and Flask are installed (`pip install flask opencv-python psutil numpy`). Then run:

  ```
  python app.py
  ```

  Note: The file was renamed from `import socket.py` to `app.py` to avoid import conflicts with Python's stdlib `socket` module.

- Configure the target VM: edit `VM_IP` in `app.py` to point to the Windows VM IP (the C++ server). Default is `127.0.0.1` for local testing. Ports must match the C++ `constants.h`.

- Templates/static: `index()` calls `render_template('dashboard.html')`. Ensure `templates/dashboard.html` and any referenced static assets exist under `templates/` and `static/` respectively during development.

- Debugging network issues:
  - Use `tcpdump` / Wireshark on the VM to confirm the C++ server binds the expected ports.
  - Confirm text commands produce expected replies on COMMAND_PORT (use `nc` or a short Python socket client).
  - For DATA_PORT, follow the header contract above; verify `HEADER_FORMAT` matches the C++ packing (endianness and field sizes).
  - Check persistent connection status with debug logs: `[DEBUG] Creating new command socket connection`.

## API endpoints (complete list)

### Core endpoints
- `GET /` — render dashboard
- `GET /api/ping` — health check
- `GET /api/<type>` — list processes or apps (type: 'processes' | 'apps')
- `POST /api/control` — execute control command (start/stop/restart)
- `POST /api/kill` — kill process by PID or app name
- `GET /api/screenshot` — capture screenshot (query param `save`: true/false)

### Media streaming
- `GET /video_feed` — webcam MJPEG stream
- `POST /api/webcam/record` — start/stop video recording (saves to `recordings/`)
- `POST /api/webcam/off` — stop webcam stream
- `GET /screen_feed` — screen MJPEG stream (real-time desktop capture)
- `POST /api/screen/stream` — start/stop screen streaming

### Keylogger
- `POST /api/keylog/toggle` — start/stop keylogger
- `GET /api/keylog/stream` — SSE stream of keystrokes
- `GET /api/keylog/stats` — get keylogger statistics

### System monitoring
- `GET /api/system/stats` — system stats (CPU, RAM, disk, network, uptime)
- `GET /api/apps/recent` — recent apps list (hardcoded for now)
- `GET /api/network/stats` — detailed network statistics with speed calculation

## Project-specific conventions & gotchas

- The Flask app does not use Blueprints — it's a single-script app. Add routes in the same file or refactor to modules carefully.
- The project expects binary payloads to be collected in chunks using a fixed header. The header format is crucial: any mismatch with C++ packing will break parsing.
- **Endianness**: Changed from little-endian (`=HHIIIII`) to big-endian (`>HHIIIII`) to match standard network byte order. Ensure C++ server uses `htons`/`htonl` when packing headers.
- Timeouts are explicitly set in a few places (command socket timeout 3s, data socket timeout 15s). Tests or automation should account for these waits.
- The code uses `errors='ignore'` when decoding received bytes to UTF-8; this is an explicit choice to tolerate non-UTF payloads (e.g., mixed binary). Be careful when adding logging that assumes valid UTF-8.
- `silent=True` parameter in `receive_large_data()` suppresses debug logs for high-frequency operations (screen streaming).
- Log suppression: Auto-refresh endpoints (`/api/system/stats`, `/api/keylog/stats`, `/api/apps/recent`, `/api/screenshot`) are filtered from Flask's request logs to reduce noise.

## Features & capabilities

### Video recording
- Webcam recording: saves to `recordings/webcam_YYYYMMDD_HHMMSS.avi`
- Uses OpenCV VideoWriter (XVID codec, 20 FPS)
- Start/stop controlled via `/api/webcam/record`

### Screen streaming
- Real-time desktop capture at 10 FPS
- Uses polling approach: repeatedly calls `SCREEN_CAPTURE` command
- Streams MJPEG to `/screen_feed`
- Silent mode to reduce log spam

### System monitoring
- Fallback to local `psutil` stats if C++ server fails
- Tracks network speed (requires tracking previous values)
- Global variables: `_last_net_io`, `_last_net_time`

### Keylogger tracking
- Global `_keylog_stats` dict tracks:
  - `total_keys`: lifetime count
  - `session_keys`: current session count
  - `last_activity`: timestamp of last keystroke
  - `is_running`: active status

## Integration points / external dependencies

- C++ server (external): required and authoritative for most flows. Keep any local changes in sync with C++ `constants.h` (ports, header packing, command names).
- Flask (PyPI) — web service dependency
- OpenCV (cv2) — video recording and frame processing
- psutil — system monitoring fallback
- numpy — image array processing

## Examples to copy when extending

- To trigger a CSV process list and parse: call `receive_large_data('PROCESS_LIST')`, then decode and split lines; the app expects first line header and comma CSV rows.
- To add a new command with a response: use `send_command_packet('MYCMD args')` and handle the returned ACK string.
- To add a new streaming endpoint: follow the pattern in `webcam_stream()` or `screen_stream_simple()`.
- To add silent operations: pass `silent=True` to `receive_large_data()`.

## Minimal checklist for contributors

1. Ensure `VM_IP` and ports match the C++ server.
2. Ensure `templates/dashboard.html` and `static/` assets exist.
3. Install dependencies: `pip install flask opencv-python psutil numpy`.
4. Run the Flask file: `python app.py` (runs on port 5001).
5. Validate binary protocols (header packing and livestream frame size) against the C++ server.
6. **Verify endianness**: Ensure C++ uses big-endian (network byte order) for DATA_PORT headers.
7. Test persistent connections: ensure reconnection works after C++ server restart.

## Known issues & TODOs

- `APP_STOP` command may receive system stats JSON instead of ACK (validation added to filter this).
- Recent apps endpoint returns hardcoded data (should integrate with C++ server).
- Network speed calculation requires global state tracking (consider refactoring).
- File naming: `app.py` is better than `import socket.py` (already fixed).

---
If any items above are unclear or you want this file expanded with diagrams, a port-mapping checklist, or error handling guide, tell me which and I will iterate.
