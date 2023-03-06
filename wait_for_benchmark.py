#!/usr/bin/python3

import socket
import time

BACKOFF_TIME=5

s = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
message = "Still working..."
s.settimeout(BACKOFF_TIME)

while True:
    s.sendto(message.encode('utf-8'), ("localhost", 1337))
    try:
        data, address = s.recvfrom(4096)
    except TimeoutError:
        print("Benchmark completed!!!")
        exit(0)
    print("Benchmark is running...")
    time.sleep(BACKOFF_TIME)

