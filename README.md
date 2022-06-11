# MPAI-AIF v1.0 implementation for the ST IoT NODE B-L475E-IOT01A (based on PlatformIO)

This code refers to the first implementation of the MPAI-AIF specification as described in https://mpai.community/wp-content/uploads/2021/10/MPAI-AIF-WD0.12.pdf. It contains a given number of AI modules (AIMs) and implements a simple use case.

The software runs on the ST IoT NODE https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html

The Architecture has the following characteristics:
- based on Zephyr operating system (based on RTOS)
- Implements only MPAI Events (High Priority events)
- Implements messages and a message store according to the MPAI-AIF 1.0 spec
- Implements great part the MPAI APIs in the form of libraries
- Implements a communication interface via IP based on CoAP  (Constrained Application Protocol) that simulates the MPAI Store.
- Parses json files according to the MPAI-AIF V 1.0 specification
- All the code is written in C language.

In order to test the architecture few tests AIMs have been implemented:

**AIM Producers**
1. Read data from sensors 
2. Does some level of processing
3. Produces an output as a message
4. Pass the message on a message store and make it ready for consumption 

**AIM Consumers**
1. Read data as message from the message store 
2. Produces an output

## BRIEF DESCRIPTION OF BOOT PROCESS
Implementing the MPAI-AIF specification, the system at the boot time:
- Reads the AIF configuration from MPAI Store
- Reads the AIW configuration (in this case *IOT-REV* AIW) from MPAI Store:
    - AIW name
    - Topology, identifying which channel is connected with respective AIM
    - List of AIM's used
- For each AIM:
    - Reads the configuration from MPAI Store    
    - Initialize it
    - Start it


## BRIEF DESCRIPTION OF USE CASE

A use case for testing the MPAI-AIF implementation has been identified. 
We call this use case the rehabilitation UC in which specific movements need to be performed in sync with the audio clue. 
The system listens via the ASC AIM to the audio signal for specific patterns (i.e., a low frequency impulse coming from a metronome) and simultaneously monitor the movement patterns via the Human Activity Classification HAC AIM. 
An additional AIM monitors if the movement is detected as correct and executed in synchronization with the specific audio pattern. 
- In affirmative case outputs a message via the serial port AIM. 
- In negative case outputs a message via the serial port AIM and make the leds blink to alert the user about the error.

### Technical overview:

The *IOT-REV AIW* ("Context-based Audio Enhancement" for "Rehabilitation Exercises Validation") is described by this JSON according with MPAI-AIF specification 1.0 and can be downloaded [here](/docs/mpai_aiw_iot_rev.json): 

The MPAI AIW consists of 4 AIMs:
1. *VolumePeaksAnalysis* ([json](/docs/mpai_aim_VolumePeaksAnalysis.json)): uses microphones to identify audio pattern (in this case volume peaks) and publish it to the channel `MicPeakDataChannel` of the message store
2. *ControlUnitSensorsReading* ([json](/docs/mpai_aim_ControlUnitSensorsReading.json)): reads data from all device sensors (like temperature, acceleration, pressure and others) and publish the values to the channel `SensorsDataChannel` of the message store
3. *MotionRecognitionAnalysis* ([json](/docs/mpai_aim_MotionRecognitionAnalysis.json)):  uses data from inertial unit, coming from `SensorsDataChannel`, to detect motion events such as start, stop etc and publish them to channel `MotionDataChannel` of the message store
4. *MovementsWithAudioValidation* ([json](/docs/mpai_aim_MovementsWithAudioValidation.json)): uses data, cross-referencing it between `MotionDataChannel` and `MicPeakDataChannel`, to recognize if the movement is done in a correct way. In particular, detects a stop event and waits for a volume peak at maximum for 1sec (configurable). It also quick blinks the leds to alert the error.

# MPAI STORE SIMULATION
Currently the MPAI STORE functionality is simulated via the delivery over CoAP/IP of the description of the use case in json. The corresponding AIMs are already resident on the board. A CoAP server that simulates the MPAI STORE is provided in Java.
The source code can be found [here](https://github.com/dbortoluzzi/mpai_store_coap_server) or downloaded [here](/executable/coap-server-0.0.1-SNAPSHOT.jar).

In order to run it ($IP_ADDRESS is the CoAP endpoint):

```bash
java -Dmpai.store.host=$IP_ADDRESS -jar coap-server-0.0.1-SNAPSHOT.jar
```

# INSTALLATION  
1. Install PlatformIO Core [here](http://docs.platformio.org/page/core.html)
2. Install dependencies:
    - cmake (3.20.0 or above)
    - python (3.6 or above)
3. Install udev rules from [here](https://docs.platformio.org/en/latest//faq/general.html#platformio-udev-rules)
4. Run (if requested) [MPAI Server CoAP](https://github.com/dbortoluzzi/mpai_store_coap_server)
5. Edit configuration on `zephyr/prj.conf`, setting the IP address of mpai coap server
   
```yaml

    CONFIG_COAP_SERVER_IPV4_ADDR="<IP_ADDRESS>" 
```
  
6. Configure WLAN (if requested), creating a file *wifi_config.c* like below:

```c

    #include "wifi_config.h"

    char* AUTO_CONNECT_SSID = "<SSID>";

    char* AUTO_CONNECT_SSID_PSK = "<PASSWORD>";
```

7. Run these commands:

```bash
    # Change directory to example
    > cd iotnode_box_test_sensors

    # Build project
    > platformio run

    # Upload firmware
    > platformio run --target upload

    # Build specific environment
    > platformio run -e disco_l475vg_iot01a

    # Upload firmware for the specific environment
    > platformio run -e disco_l475vg_iot01a --target upload

    # Clean build files
    > platformio run --target clean
```

# Licence
Licence information for each components of this example is detailed in [LICENCE.MD](/LICENCE.md)
