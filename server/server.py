from Packet import *
import socket
import sys


class Server:
    def __init__(self, threshold: float = 5, router_ip: str = "127.0.0.1", router_port: int = 8014):
        self.values = {}
        self.threshold = threshold
        self.router_ip = router_ip
        self.router_port = router_port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.connect((self.router_ip, self.router_port))
        self.sock.send("Connecting...".encode())

    def handle_received_data(self, packet: DataPacket):
        values_list = self.values.get(packet.address, [])
        if len(values_list) >= 30:
            values_list = values_list[1:]

        values_list.append(packet.data)
        self.values[packet.address] = values_list

        if len(values_list) > 10 and self.compute_slope(packet.address) > self.threshold:
            print(" Sending OPEN message to node [{node}]".format(node=packet.address), end="")
            self.send_packet(OpenPacket(packet.address))

    def compute_slope(self, node: int):
        values = self.values.get(node, [])
        slope = least_squares_slope(values)
        print(" Slope", slope, end="")
        return (int(slope * 100)) / 100

    def send_packet(self, packet: Packet):
        self.sock.send(packet.encode())

    def receive_packet(self):
        data = self.sock.recv(10)
        packet: DataPacket = PackFactory.parse_packet(data)
        print("\nReceived:", packet.address, packet.data, end="")
        self.handle_received_data(packet)


def least_squares_slope(y: list):
    n = len(y)
    x = range(n)
    sum_x, sum_y = sum(x), sum(y)
    sum_xy, sum_xx = 0, 0

    for index in x:
        sum_xy += x[index] * y[index]
        sum_xx += x[index] * x[index]
    slope = (sum_x * sum_y - n * sum_xy) / (sum_x * sum_x - n * sum_xx)

    return (int(slope * 100)) / 100


if __name__ == '__main__':
    threshold = 5
    try:
        router_ip = sys.argv[1]
        router_port = int(sys.argv[2])
    except:
        print("The server takes at least 2 parameters:")
        print("[Mandatory]\tRouter IP [string] (IPv6 address)")
        print("[Mandatory]\tRouter PORT [int] (UDP port)")
        print("[Optional]\tSlope threshold [float] (minimum slope to trigger valves opening)")
        exit(-1)

    if len(sys.argv) > 3:
        # This threshold corresponds to the slope threshold
        threshold = sys.argv[3]

    server = Server(float(threshold))
    i = 0
    while True:
        server.receive_packet()
