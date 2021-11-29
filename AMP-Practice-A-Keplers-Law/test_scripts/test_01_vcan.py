import os
import struct
import subprocess
import socket

can_frame_fmt = "=IB3x8s"
can_frame_size = struct.calcsize(can_frame_fmt)


interface = "vcan0"

def build_can_frame(can_id, data):
    can_dlc = len(data)
    data = data.ljust(8, b'\x00')
    return struct.pack(can_frame_fmt, can_id, can_dlc, data)

def dissect_can_frame(frame):
    can_id, can_dlc, data = struct.unpack(can_frame_fmt, frame)
    return (can_id, can_dlc, data[:can_dlc])

def test_vcan_kernel():
    assert interface in subprocess.check_output("ifconfig").decode('utf-8')

def test_open_socket():
    # create a raw socket and bind it to the 'vcan0' interface
    s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    s.bind((interface,))
    can_id = 0x92345678
    data = b'12345678'
    cf = build_can_frame(can_id, data)
    assert s.send(cf) == can_frame_size
    