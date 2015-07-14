#!/usr/bin/env python3

import zmq

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect('tcp://127.0.0.1:5556')
socket.setsockopt_string(zmq.SUBSCRIBE, '')

for i in range(1000):
    print(i, len(socket.recv()))
