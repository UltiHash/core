#!/usr/bin/python3

import threading
import sys
import socket
import time

class Spinner(threading.Thread):

    def __init__(self):
        super().__init__(target=self._spin)
        self._stopevent = threading.Event()

    def stop(self):
        self._stopevent.set()

    def _spin(self):

        while not self._stopevent.is_set():
            for t in '|/-\\':
                sys.stdout.write(t)
                sys.stdout.flush()
                time.sleep(0.1)
                sys.stdout.write('\b')



BACKOFF_TIME=5

if __name__ == '__main__':
    s = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    message = "Still working..."
    s.settimeout(BACKOFF_TIME)

    sys.stdout.write("Instantiated test setup, now waiting for its completion... ")
    spinner = Spinner()
    spinner.start()
    while True:
        s.sendto(message.encode('utf-8'), ("localhost", 1337))
        try:
            data, address = s.recvfrom(4096)
        except TimeoutError:
            spinner.stop()
            sys.stdout.write('\b')
            sys.stdout.flush()
            sys.stdout.write('\n')
            sys.stdout.flush()
            exit(0)
        time.sleep(BACKOFF_TIME)

