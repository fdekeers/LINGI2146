import time

DATA_PACKET = 0
OPEN_PACKET = 1

HEX_SIZE_OF_TYPE = 2
HEX_SIZE_OF_ADDR = 4
HEX_SIZE_OF_DATA = 2

class Packet:
    def __init__(self, address):
        self.address = address
        self.type = -1
        self.time = round(time.time())

    def encode(self):
        """
        Encodes the packet
        :return: the encoded packet using format TYPE/ADDRESS
        """
        if self.type < 0:
            return "0"
        return "{}/{}".format(self.type, self.address)


class DataPacket(Packet):
    def __init__(self, src_addr, data):
        super().__init__(src_addr)
        self.data = data
        self.type = DATA_PACKET

    def encode(self):
        """
        Encodes the packet
        :return: the encoded packet using format TYPE/ADDRESS/DATA
        """
        return "{}/{}".format(super().encode(), self.data)


class OpenPacket(Packet):
    def __init__(self, dst_addr):
        super().__init__(dst_addr)
        self.type = OPEN_PACKET


class PackFactory:
    @staticmethod
    def parse_packet(raw_packet):
        """
        Parses the raw_packet to return a Packet or None if the raw_packet does not correspond to a packet
        :param raw_packet: the packet received over the socket connection
        :return: Packet|None
        """
        packet = raw_packet.decode("utf-8").split('/')
        try:
            packet_type = int(packet[0])

            if packet_type == DATA_PACKET:
                src_addr = int(packet[1])
                data = int(packet[2])
                return DataPacket(src_addr, data)
            elif packet_type == OPEN_PACKET:
                dst_addr = packet[1]
                return OpenPacket(dst_addr)
            else:
                return None
        except Exception:
            return None