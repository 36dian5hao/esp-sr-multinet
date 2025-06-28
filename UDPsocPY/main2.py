import socket
import pyaudio

# 服务器的 IP 地址和端口号
SERVER_IP = '0.0.0.0'
SERVER_PORT = 8888

# 创建 UDP 套接字
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 绑定 IP 地址和端口号
server_socket.bind((SERVER_IP, SERVER_PORT))

print(f"UDP 服务器正在监听 {SERVER_IP}:{SERVER_PORT}...")

# 初始化 PyAudio
p = pyaudio.PyAudio()

# 音频参数
FORMAT = pyaudio.paInt16  # 音频格式
CHANNELS = 1  # 声道数
RATE = 44100  # 采样率
CHUNK = 1024  # 缓冲区大小

# 打开音频流
stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                output=True,
                frames_per_buffer=CHUNK)

try:
    while True:
        # 接收音频数据
        data, client_address = server_socket.recvfrom(CHUNK)
        print(f"从 {client_address} 接收到 {len(data)} 字节的音频数据")

        # 播放音频数据
        stream.write(data)

except KeyboardInterrupt:
    print("程序终止，正在清理资源...")
finally:
    # 关闭音频流和 PyAudio
    stream.stop_stream()
    stream.close()
    p.terminate()

    # 关闭 UDP 套接字
    server_socket.close()