DATA_PACKET = 0
OPEN_PACKET = 1

HEX_SIZE_OF_TYPE = 2
HEX_SIZE_OF_ADDR = 4
HEX_SIZE_OF_DATA = 2

FROM_BINARY = False


class Packet:
    def __init__(self, address):
        self.address = address
        self.type = -1

    def encode(self):
        if self.type < 0:
            return "0"
        if FROM_BINARY:
            return self.type.to_bytes(HEX_SIZE_OF_TYPE, 'little') + self.address.to_bytes(HEX_SIZE_OF_ADDR, 'little')
        else:
            return "{}/{}".format(self.type, self.address)


class DataPacket(Packet):
    def __init__(self, src_addr, data):
        super().__init__(src_addr)
        self.data = data
        self.type = DATA_PACKET

    def encode(self):
        if FROM_BINARY:
            return super().encode() + self.data.to_bytes(HEX_SIZE_OF_DATA, 'little')
        else:
            return "{}/{}".format(super().encode(), self.data)


class OpenPacket(Packet):
    def __init__(self, dst_addr):
        super().__init__(dst_addr)
        self.type = OPEN_PACKET


class PackFactory:
    @staticmethod
    def parse_packet(raw_packet):
        if FROM_BINARY:
            try:
                packet_type = int(raw_packet[0:HEX_SIZE_OF_TYPE].decode("ascii"), 16)
            except ValueError:
                packet_type = -1
            if packet_type == DATA_PACKET:
                src_addr = int(raw_packet[HEX_SIZE_OF_TYPE:HEX_SIZE_OF_TYPE + HEX_SIZE_OF_ADDR].decode("ascii"), 16)
                data = int(
                    raw_packet[HEX_SIZE_OF_TYPE + HEX_SIZE_OF_ADDR:HEX_SIZE_OF_TYPE + HEX_SIZE_OF_ADDR + HEX_SIZE_OF_DATA].decode("ascii"),
                    16
                )
                return DataPacket(src_addr, data)
            elif packet_type == OPEN_PACKET:
                dst_addr = int.from_bytes(raw_packet[HEX_SIZE_OF_TYPE:HEX_SIZE_OF_TYPE + HEX_SIZE_OF_ADDR], 'little')
                return OpenPacket(dst_addr)
            else:
                return None
        else:
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