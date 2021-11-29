import os
import struct
import subprocess
import socket

def get_can_interface():
    try:
        with open('/home/debian/AMP-Challenge-A-Keplers-Law/can_interface.txt','r') as f:
            interface = f.read()
    except FileNotFoundError:
        interface = 'vcan0'
    return interface
    
interface = get_can_interface()
s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
s.bind((interface,))
can_frame_fmt = "=IB3x8s"
can_frame_size = struct.calcsize(can_frame_fmt)
    
def build_can_frame(can_id, data):
    can_dlc = len(data)
    data = data.ljust(8, b'\x00')
    return struct.pack(can_frame_fmt, can_id, can_dlc, data)

def dissect_can_frame(frame):
    can_id, can_dlc, data = struct.unpack(can_frame_fmt, frame)
    return (can_id, can_dlc, data[:can_dlc])

def test_build_kepler_server():
    os.chdir("BBB")
    assert "src/kepler_server.c" in subprocess.check_output("make").decode('utf-8')

def kill_kepler_server():
    # Check to see if the kepler_server process is running, then kill it.
    output = subprocess.check_output("ps -e".split()).decode('utf-8')
    if "kepler_server" in output:
        processes = output.split('\n')
        for p in processes:
            if "kepler_server" in p:
                line = p.split()
                os.system("kill -9 {}".format(line[0]))
    
def test_launch_kepler_server():
    kill_kepler_server()
    # Make sure there is no process running
    assert "kepler_server" not in subprocess.check_output("ps -e".split()).decode('utf-8')
    #Launch an new instance of the server in the background
    os.system("build/kepler_server {} &".format(interface))
    # Make sure the process is running
    assert "kepler_server" in subprocess.check_output("ps -e".split()).decode('utf-8') 
    