import random
import socket
from Packet import *
import select

if __name__ == '__main__':
    udp_ip = '127.0.0.1'
    udp_port = 8014
    fd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    fd.bind((udp_ip, udp_port))
    msg, addr = fd.recvfrom(100)
    print("Received", msg.decode())
    fd.connect(addr)

    print("Connected")
    while True:
        data_packet = DataPacket(random.randint(0, 3), random.randint(0, 255))
        print("Sending: DATA", data_packet.address, data_packet.data)
        fd.sendto(data_packet.encode(), addr)
        ready = select.select([fd], [], [], 0.5)
        if ready[0]:
            open_packet: OpenPacket = PackFactory.parse_packet(fd.recv(10))
            print("Received: OPEN", open_packet.address)
