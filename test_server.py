import socket
import time

HOST = '127.0.0.1'
PORT = 8888


def recv_line(sock):
    data = b''
    while not data.endswith(b'\n'):
        chunk = sock.recv(1)
        if not chunk:
            raise RuntimeError('Server đã ngắt kết nối')
        data += chunk
    return data.decode('utf-8', 'ignore')


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect((HOST, PORT))

    time.sleep(0.2)
    header = recv_line(s)
    print('HEADER:', header.strip())

    time.sleep(0.2)
    data_line = recv_line(s)
    print('DATA:', data_line.strip())

    nums = list(map(int, data_line.strip().split()))
    total = sum(nums)
    s.sendall(f'RESULT {total}\n'.encode('utf-8'))

    time.sleep(0.2)
    resp1 = recv_line(s)
    print('RESP1:', resp1.strip())

    time.sleep(0.2)
    resp2 = recv_line(s)
    print('RESP2:', resp2.strip())

    s.close()


if __name__ == '__main__':
    main()
