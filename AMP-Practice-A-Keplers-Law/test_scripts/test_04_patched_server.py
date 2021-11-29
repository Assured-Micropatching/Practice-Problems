import os
import subprocess

from test_02_kepler_server import kill_kepler_server, get_can_interface

interface = get_can_interface()

def test_launch_patched_server():
    # Start from a clean slate
    kill_kepler_server()
    # Make sure there is no process running
    assert "kepler_server" not in subprocess.check_output("ps -e".split()).decode('utf-8')
    # Launch the source code patched version to see that it works
    os.chdir("/home/debian/AMP-Challenge-A-Keplers-Law/BBB")
    os.system("build/kepler_server_patch_1 {} &".format(interface))
    # Make sure the process is running
    assert "kepler_server_p" in subprocess.check_output("ps -e".split()).decode('utf-8')
    
  