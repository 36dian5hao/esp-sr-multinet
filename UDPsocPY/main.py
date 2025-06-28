
import socket
 
# 创建一个UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
 
# 绑定到本地 IP 和端口
local_addr = ('192.168.1.4', 8888)
udp_socket.bind(local_addr)
 
print('UDP server is listening...')
 
while True:
    # 接收数据
    data, addr = udp_socket.recvfrom(1024)
    print(f'Received data from {addr}: {data.decode()}')
 
    # 回复数据
    reply = 'Received: ' + data.decode()

