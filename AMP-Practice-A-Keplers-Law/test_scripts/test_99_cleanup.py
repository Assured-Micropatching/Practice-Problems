import subprocess

from test_02_kepler_server import kill_kepler_server   

def test_cleanup():
    kill_kepler_server()
    assert "kepler_server" not in subprocess.check_output("ps -e".split()).decode('utf-8')
    