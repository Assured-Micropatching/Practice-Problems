# AMP-Challenge-A-Keplers-Law
Code written and compiled on AMP hardware to solve Kepler's Law.

Keplers law is 

`M = E - e sin(E)`

where `M` is the mean anomaly, `E` is the eccentric anomaly, and `e` is the eccentricity. `E` and `M` are in units of radians.

https://en.wikipedia.org/wiki/Kepler%27s_equation

To solve this equation, we have to use a numerical method and iterations. 

## Getting Things Working

### TL;DR
On the BeagleBone Black, connected to the Internet, open a shell and navigate to this directory. Then,
```
sudo vcan_setup.sh
pip3 install -r requirements.txt
python3 -m pytest
```
### System Testing Approach
The pytest framework runs the test scripts, which will compile the code for both the patched and vulnerable program, test the virtual CAN channel, and run the test cases. The interface for invoking the program is over CAN, so if the response on CAN doesn't arrive, the socket times out and the program is killed. This is supposed to happen when testing the vulnerable program and does not happen when testing the patched program. 

The patch for this issue was to simply increase the tolerance for convergence by an order of magnitude. Other source code patches are available in commentted out sections of the program source code.

If the test script above fails, look at the output and see what test failed. The most likely failure is the lack of a valid CAN channel. 




## Story 

NASA's [MMS Spacecraft fleet](https://mms.gsfc.nasa.gov/) experienced an anomaly in their orbit determination algorithms that caused an infinite loop. The software team spent days searching for the culprit and quickly found that the convergence tolerance was set to machine epsilon!

[Read more](story.md) about the the NASA mission that inspired this challenge problem.

![NASA Image](MMSinSpace_small.jpg)

## Inputs and Outputs
The data needed for Kepler's Law formulation as inputs is the mean anomaly, M, and eccentricity, e. Since we are looking at elliptic orbits, the eccentricity is bounded between 0 and 1. 

Since the AMP challenge problems make use of the CAN bus for networking, we'll define the parameters using the following parameter group numbers in SAE J1939.

Each parameter will use a pre-existing SLOT defined in J1939.
SLOT is the acronym for Scaling, Limit, Offset and Transfer function.


### Input Data PGN: 0x1FF14
The input data are the mean anomaly, M, and the eccentricity, e. We have 8 bytes to encode this data. In keeping with the J1939 standard, we'll encode the data as unsigned integers in Intel format, then apply a scale and offset in the program to convert it to a floating point number. 

The message will be constructed as follows :

| CAN ID | DLC | Data Bytes |
| --- | --- | --- |
| 19FF1400 | 8 | LSB of M, Byte 1 of M, Byte 2 of M, MSB of M, LSB e, MSB of e, Reserved |

The eccentricity will be represented as a percentage and make use of the SLOT in SAE J1939 used for representing factors that go from 0 to 100%. We will use slot number 237 is SAE J1939. 

| Parameter | SLOT Identifier | SLOT Name | SLOT Type | Scaling | Range | Offset | Length |
| --- | --- | --- | --- | --- | --- | --- | --- |
| e | 345 | SAEpc21 | Percent | 0.0015625 %/bit | 0 to 100.3984375 % | 0 | 2 bytes |
| M | 7	| SAEad01 | Angle/Direction | 0.0000001 deg/bit | -210 to 211.1081215 deg | -210 deg | 4 bytes |

Note: These SLOTS are just for encoding the CAN message. The internal computaions will require the use of radians. 



### Output Data PGN: 0x1FF15
The output will be the Eccentric Anomaly, E, along with the computation diagnostics. Computation diagnostics will not be included in the original binary, but will be used in the patched binary.


#### Vulnerable
 The output message will be constructed as follows (for vulnerable code):

| CAN ID | DLC | Data Bytes |
| --- | --- | --- |
| 19FF1500 | 8 | LSB of E, Byte 1 of E, Byte 2 of E, MSB of E,  Reserved |

The reserved field will be set to all 1's (0xFFFFFFFF).

Since E is an angle measure (computed in radians),  will follow the same SLOT definition as M, which is SAE J1939 SLOT number 7.

#### Patched code

| CAN ID | DLC | Data Bytes |
| --- | --- | --- |
| 19FF1500 | 8 | LSB of E, Byte 1 of E, Byte 2 of E, MSB of E,  status, iter., microseconds |

There are three ways to patch the binary:
1. change the tolerance to be looser with the exit criteria (i.e. patch the data defining the convergence tolerance).
2. add an iteration counter and exit after there are too many iterations (i.e. N_max = 50).
3. add a loop timer and exit after a certain amount of time has passed (i.e. T_max = 250usec).


The first patch candidate is the simplest and would be a minimal change in the binary. However, certain input conditions may still lead to an infinite loop. How can we be sure the patch will be robust?

The second patch will require the addition of a counter and exit criteria check in the loop. This will add additional instructions and variables to the binary. Doing this (without a source code patch) is one of the goals of AMP. Please try to work through this patch. 

The third patch is similar to the second because additional variables and code are needed. 

There may be other ways to fix the problem. Please let everyone know if you have an alternative approach.

## Jupyter Notebook Python Code
An implementation of Kepler's Law for python in a Jupyter Notebook is provided as a proof of concept for the problem and patch.

## Beagle Bone Black
### Command Line based program
```
debian@beaglebone:~/AMP-Challenge-A-Keplers-Law/BBB$ build/kepler_server_patch_1 vcan0
```

### CAN bus capable
Server C Code
The C code in this repository is a kepler's law calculation implemented as a server accepting requests over CAN. The server is meant to be run with the command line with command line arguments. It is built with socketCAN and C standard libraries. 

The server code expects the following command line arguments:

 - CAN Channel: can0,can1,vcan0,vcan1, etc.

The Kepler's law calculation uses the given mean anomaly for the inital guess. The server responds to requests with the converged value or goes into an infinite loop. The process must be killed if the server goes into an infinite loop.

### Test Framework
The pytest framework is used to automate the build process, run the test cases, and kill the process. The test scripts also act as a python client in this case.
