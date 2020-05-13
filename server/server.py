from Packet import *
import socket
import sys


class Server:
    def __init__(self, threshold=5, router_ip="127.0.0.1", router_port=60001):
        self.values = {}
        self.threshold = threshold
        self.router_ip = router_ip
        self.router_port = router_port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.router_ip, self.router_port))

    def handle_received_data(self, packet):
        """
        Interprets the received data packet
        :param packet: received data packet
        :return: None
        """
        values_list = self.values.get(packet.address, [])

        if (packet.time, packet.data) == values_list[-1]:  # The data is a duplicate (due to runicast ack losses)
            return

        if len(values_list) >= 30:
            values_list = values_list[1:]

        values_list.append((packet.time, packet.data))
        self.values[packet.address] = values_list

        if len(values_list) > 10 and self.compute_slope(packet.address) > self.threshold:
            print("Sending OPEN message to node [{node}]".format(node=packet.address))
            self.send_packet(OpenPacket(packet.address))

    def compute_slope(self, node):
        """
        Computes slope of the least square regression of the data sent by a node
        :param node: node we want to compute the regression for
        :return: the slope of the regression
        """
        values = self.values.get(node, [])
        slope = least_squares_slope(values)
        return (int(slope * 100)) / 100

    def send_packet(self, packet):
        """
        Send a packet over the socket connection
        :param packet: packet to send
        :return: None
        """
        self.sock.send(bytes(packet.encode(), "utf-8") + b"\n")

    def receive_packet(self):
        """
        Gets packets from the socket connection and triggers the handling of those
        :return: None
        """
        data = self.sock.recv(1)
        buf = b""
        while data.decode("utf-8") != "\n":
            buf += data
            data = self.sock.recv(1)
        packet = PackFactory.parse_packet(buf)
        if packet is not None:
            print("Received data: \tADDR = {}\tDATA = {}\tTIME = {}".format(packet.address, packet.data, packet.time))
            self.handle_received_data(packet)


def least_squares_slope(tuples):
    """
    Computes the slope of the least square regression of the tapple list given as parameter
    :param tuples: the (x, y) data
    :return: the slope
    """
    n = len(tuples)
    x = [t[0] for t in tuples]
    y = [t[1] for t in tuples]
    sum_x, sum_y = sum(x), sum(y)
    sum_xy, sum_xx = 0, 0

    for index in range(n):
        sum_xy += x[index] * y[index]
        sum_xx += x[index] * x[index]
    slope = (sum_x * sum_y - n * sum_xy) / (sum_x * sum_x - n * sum_xx)

    return (int(slope * 100)) / 100


if __name__ == '__main__':
    threshold = 5
    try:
        router_ip = sys.argv[1]
        router_port = int(sys.argv[2])
    except Exception:
        print("The server takes at least 2 parameters:")
        print("[Mandatory]\tRouter IP [string] (IPv6 address)")
        print("[Mandatory]\tRouter PORT [int] (UDP port)")
        print("[Optional]\tSlope threshold [float] (minimum slope to trigger valves opening)")
        exit(-1)

    if len(sys.argv) > 3:
        # This threshold corresponds to the slope threshold
        threshold = sys.argv[3]

    server = Server(int(threshold))
    i = 0
    while True:
        server.receive_packet()
