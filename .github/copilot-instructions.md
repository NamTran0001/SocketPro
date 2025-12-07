## Quick context

This repository contains a small Python Flask web UI that communicates with a companion C++ server over raw TCP sockets. The Python side acts as a control panel and media bridge (screenshots, process lists, webcam MJPEG, keylogger SSE) for a Windows VM. The Flask app is the single web-facing service; the C++ server holds device/OS-specific logic and serves binary payloads on distinct ports.

## Big-picture architecture

- Flask web server (file: `import socket.py`) — HTTP UI + API endpoints.
- C++ server (separate repo/binary) — listens on multiple TCP ports. Python connects as a client to:
  - COMMAND_PORT (8888) — short textual commands + optional ACK responses
  - DATA_PORT (8889) — large binary payloads sent with a fixed header
  - LIVESTREAM_PORT (8890) — MJPEG frames with a 4-byte size prefix per JPEG
  - KEYLOG_PORT (8891) — streaming text (SSE events forwarded to browser)

Why this split: the C++ side performs privileged / platform-specific capture and control; Python provides a developer-friendly web UI and HTTP API surface.

## Key files & symbols (what to read first)

- `import socket.py` — main Flask app. Implements: `send_command_packet`, `receive_large_data`, `webcam_stream`, SSE keylog streaming endpoints, and API routes like `/api/list/<type>`, `/api/screenshot`, `/video_feed`, `/stream/keylog`.
- `templates/dashboard.html` — referenced by `index()` (not included in current snapshot). The app expects a `templates` directory for Flask rendering.
- Constants to keep in sync with C++: `VM_IP`, `COMMAND_PORT`, `DATA_PORT`, `LIVESTREAM_PORT`, `KEYLOG_PORT`, and `HEADER_FORMAT` / `HEADER_SIZE`.

## Protocol / data contracts (explicit)

- Header for large DATA channel (24 bytes): `HEADER_FORMAT = '=HHIIIII'` (packed as uint16, uint16, uint32, uint32, uint32, uint32, uint32). `HEADER_SIZE = 24`.
  - Fields unpacked as: `cmd, res, total, curr, size, total_size, checksum` in `receive_large_data()`.
  - Python expects chunks: header then `size` bytes of payload (loop until `curr >= total - 1`).

- Livestream protocol: server sends 4-byte signed int (C `int`) size then JPEG bytes. Python unpacks with `struct.unpack('i', size_data)`.

- Commands sent over COMMAND_PORT are plain ASCII strings, examples used by the app:
  - `PING` — health check
  - `PROCESS_LIST`, `APP_LIST` — triggers server to send CSV to DATA_PORT
  - `SCREEN_CAPTURE` — server sends a JPEG payload over DATA_PORT
  - `LIVESTREAM` / `STOPLIVESTREAM` — control livestream
  - `KEYLOG` / `STOPKEYLOG` — start/stop keylogger stream
  - `START <target>`, `STOP <target>`, `RESTART <target>`, etc. — control action format in `/api/control`

## Developer workflows & run/debug tips

- Run locally (macOS/zsh): make sure Python 3.8+ and Flask are installed (`pip install flask`). Then run:

  python "import socket.py"

  Note: the filename contains a space in the snapshot (`import socket.py`). It is recommended to rename the file to a Python-friendly module name (e.g. `app.py` or `server_bridge.py`) to avoid shell-quoting issues and accidental import conflicts with the stdlib `socket` module. If you keep the name, always quote it when running.

- Configure the target VM: edit `VM_IP` in `import socket.py` to point to the Windows VM IP (the C++ server). Ports must match the C++ `constants.h`.

- Templates/static: `index()` calls `render_template('dashboard.html')`. Ensure `templates/dashboard.html` and any referenced static assets exist under `templates/` and `static/` respectively during development.

- Debugging network issues:
  - Use `tcpdump` / Wireshark on the VM to confirm the C++ server binds the expected ports.
  - Confirm text commands produce expected replies on COMMAND_PORT (use `nc` or a short Python socket client).
  - For DATA_PORT, follow the header contract above; verify `HEADER_FORMAT` matches the C++ packing (endianness and field sizes).

## Project-specific conventions & gotchas

- The Flask app does not use Blueprints — it's a single-script app. Add routes in the same file or refactor to modules carefully.
- The project expects binary payloads to be collected in chunks using a fixed header. The header format is crucial: any mismatch with C++ packing will break parsing.
- Timeouts are explicitly set in a few places (command socket timeout 3s, data socket timeout 15s). Tests or automation should account for these waits.
- The code uses `errors='ignore'` when decoding received bytes to UTF-8; this is an explicit choice to tolerate non-UTF payloads (e.g., mixed binary). Be careful when adding logging that assumes valid UTF-8.

## Integration points / external dependencies

- C++ server (external): required and authoritative for most flows. Keep any local changes in sync with C++ `constants.h` (ports, header packing, command names).
- Flask (PyPI) — web service dependency.

## Examples to copy when extending

- To trigger a CSV process list and parse: call `receive_large_data('PROCESS_LIST')`, then decode and split lines; the app expects first line header and comma CSV rows.
- To add a new command with a response: use `send_command_packet('MYCMD args')` and handle the returned ACK string.

## Minimal checklist for contributors

1. Ensure `VM_IP` and ports match the C++ server.
2. Ensure `templates/dashboard.html` and `static/` assets exist.
3. Run the Flask file with Python (quote filename if it contains spaces).
4. Validate binary protocols (header packing and livestream frame size) against the C++ server.

---
If any items above are unclear or you want this file expanded with diagrams, a port-mapping checklist, or a suggested refactor (rename `import socket.py` → `app.py` and add a requirements.txt), tell me which and I will iterate.
