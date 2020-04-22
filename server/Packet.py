DATA_PACKET = 0
OPEN_PACKET = 4

SIZE_OF_TYPE = 1
SIZE_OF_ADDR = 2
SIZE_OF_DATA = 2


class Packet:
    def __init__(self, address):
        self.address = address
        self.type = -1

    def encode(self) -> bytes:
        if self.type < 0:
            return b"\00"
        return self.type.to_bytes(SIZE_OF_TYPE, 'little') + self.address.to_bytes(SIZE_OF_ADDR, 'little')


class DataPacket(Packet):
    def __init__(self, src_addr: int, data: int):
        super().__init__(src_addr)
        self.data = data
        self.type = DATA_PACKET

    def encode(self) -> bytes:
        return super().encode() + self.data.to_bytes(SIZE_OF_DATA, 'little')


class OpenPacket(Packet):
    def __init__(self, dst_addr: int):
        super().__init__(dst_addr)
        self.type = OPEN_PACKET


class PackFactory:
    @staticmethod
    def parse_packet(raw_packet: bytes) -> Packet:
        packet_type: int = int.from_bytes(raw_packet[0:SIZE_OF_TYPE], 'little')
        if packet_type == DATA_PACKET:
            src_addr: int = int.from_bytes(raw_packet[SIZE_OF_TYPE:SIZE_OF_TYPE+SIZE_OF_ADDR], 'little')
            data: int = int.from_bytes(
                raw_packet[SIZE_OF_TYPE+SIZE_OF_ADDR:SIZE_OF_TYPE+SIZE_OF_ADDR+SIZE_OF_DATA],
                'little'
            )
            return DataPacket(src_addr, data)
        elif packet_type == OPEN_PACKET:
            dst_addr: int = int.from_bytes(raw_packet[SIZE_OF_TYPE:SIZE_OF_TYPE+SIZE_OF_ADDR], 'little')
            return OpenPacket(dst_addr)
