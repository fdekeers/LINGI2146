import random
import sys
from time import sleep


class Server:
    def __init__(self, threshold: int = 5):
        self.values = {}
        self.threshold = threshold

    def handle_received_data(self, node: int, value: float):
        values_list = self.values.get(node, [])
        if len(values_list) >= 30:
            values_list = values_list[1:]

        values_list.append(value)
        self.values[node] = values_list

        if len(values_list) > 10 and self.compute_slope(node) > self.threshold:
            self.send_open_valve(node)

    def compute_slope(self, node: int):
        values = self.values.get(node, [])
        slope = least_squares_slope(values)
        print(" Slope", slope, end="")
        return (int(slope * 100)) / 100

    def send_open_valve(self, node: int):
        print("Sending OPEN message to node [{node}]".format(node=node))


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
    if len(sys.argv) > 1:
        threshold = sys.argv[1]

    server = Server(int(threshold))
    i = 0
    while True:
        sleep(random.random()/10)
        print("\n", i, "Sending data", end="")
        server.handle_received_data(random.randint(0, 3), i)
        i += 1
