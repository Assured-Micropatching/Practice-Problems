import os
import struct
import subprocess
import socket
from math import pi

from test_02_kepler_server import kill_kepler_server, build_can_frame, dissect_can_frame, get_can_interface


can_frame_fmt = "=IB3x8s"
can_frame_size = struct.calcsize(can_frame_fmt)
interface = get_can_interface()
s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
s.bind((interface,))
s.settimeout(1)

def encode_degrees(angle):
    """
    Input is radians as a float.
    Output is the J1939 encoded data in 4 bytes according to the SLOT
    """
    encoded_int = int((angle + 210)*1000000)
    return struct.pack("<L",encoded_int)

def encode_ecc(ecc):
    eccentricity = int(ecc/0.0015625)
    return struct.pack("<H",eccentricity)

def request_calculation(ecc,angle):
    data = encode_degrees(angle) + encode_ecc(ecc) + b'\xFF\xFF'
    can_id = 0x18FF1417 | 0x80000000 # Set the first bit for an extended CAN ID
    cf = build_can_frame(can_id, data)
    assert s.send(cf) == can_frame_size
    #Listen from server
    new_can_id = 0
    while (new_can_id & 0x00FFFF00) != 0x00FF1500:
        response = s.recv(can_frame_size)
        new_can_id, can_dlc, new_data = dissect_can_frame(response)
    assert len(response) == can_frame_size
    #print("new_can_id: {:08X}".format(new_can_id & 0x00FFFF00))
    assert (new_can_id & 0x00FFFF00) == 0x00FF1500
    assert can_dlc == 8

    #interpret results
    ecc_anom_degrees = struct.unpack("<L",new_data[:4])[0]*0.000001 - 210
    assert ecc_anom_degrees >= -210
    assert ecc_anom_degrees <=  210

def test_lower_input_boundary():
    can_id = 0x18FF1417 | 0x80000000 # Set the first bit for an extended CAN ID
    data = b'\x00'*8 # Set to all zeros
    cf = build_can_frame(can_id, data)
    s.settimeout(1)
    assert s.send(cf) == can_frame_size
    #Listen from server
    new_can_id = 0
    while (new_can_id & 0x00FFFF00) != 0x00FF1500:
        response = s.recv(can_frame_size)
        new_can_id, can_dlc, new_data = dissect_can_frame(response)
    assert len(response) == can_frame_size
    assert (new_can_id & 0x00FFFF00) == 0x00FF1500
    assert can_dlc == 8
    

def test_upper_input_boundary():
    can_id = 0x18FF1417 | 0x80000000 # Set the first bit for an extended CAN ID
    data = b'\xFF'*8 # Set to all FFs
    cf = build_can_frame(can_id, data)
    s.settimeout(1)
    assert s.send(cf) == can_frame_size
    #Listen from server
    new_can_id = 0
    while (new_can_id & 0x00FFFF00) != 0x00FF1500:
        try:
            response = s.recv(can_frame_size)
        except Exception as e:
            assert "timeout" in repr(e)
            return
        new_can_id, can_dlc, new_data = dissect_can_frame(response)
    assert len(response) == can_frame_size
    assert new_can_id == 0x98FF1500
    assert can_dlc == 8

def test_for_vulnerable_inputs():
    can_id = 0x18FF1417 | 0x80000000 # Set the first bit for an extended CAN ID
    convergence_failure = 0
    N = 10
    for i in range(1,N+1):
        angle = 360*i/N
        ecc = .338
        try:
            request_calculation(ecc,angle)
        except Exception as e:
            print(repr(e))
            kill_kepler_server()
            convergence_failure += 1
            break
    assert convergence_failure > 0

 