from test_03_input_cases import request_calculation

def test_converged_inputs():
    N = 10
    for i in range(1,N+1):
        angle = 360*i/N
        ecc = .338
        # If there is an issue, then the socket will timeout
        request_calculation(ecc,angle)