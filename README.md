# MPAI-AIF v1.0 implementation for the ST IoT NODE B-L475E-IOT01A, (based on PlatformIO)

This code refers to the first implementation of the MPAI-AIF specification as described in https://mpai.community/wp-content/uploads/2021/10/MPAI-AIF-WD0.12.pdf. It contains a given number of AI modules (AIMs) and implements a simple use case.

The software runs on the ST IoT NODE https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html
The Architecture has the following characteristics:
- based on Zephyr operating system (based on RTOS)
- Implements only MPAI Events (High Priority events)
- Implements a message store according to the MPAI-AIF 1.0 spec
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
- Reads the AIW configuration (in this case *CAE-REV* AIW) from MPAI Store:
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

The *CAE-REV AIW* ("Context-based Audio Enhancement" for "Rehabilitation Exercises Validation") is described by this JSON according with MPAI-AIF specification:
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://mpai.community/standards/resources/MPAI-AIF/V1/AIW-AIM-metadata.schema.json",
  "title": "CAE AIF v1 AIW/AIM metadata",
  "Identifier": {
    "ImplementerID": 1,
    "Specification": {
      "Standard": "MPAI-CAE",
      "AIW": "CAE-REV",
      "AIM": "CAE-REV",
      "Version": "1"
    }
  },
  "APIProfile": "Main",
  "Description": "AIW that implements Use-Case CAE-REV (Rehabilitation Exercises Validation)",
  "Types": [
    {
      "Name": "SENSORS_DATA",
      "Type": "mpai_parser_t"
    },
    {
      "Name": "MIC_BUFFER_DATA",
      "Type": "mpai_parser_t"
    },
    {
      "Name": "MIC_PEAK_DATA",
      "Type": "mpai_parser_t"
    },
    {
      "Name": "MOTION_DATA",
      "Type": "mpai_parser_t"
    }
  ],
  "Ports": [
    {
      "Name": "SensorsDataChannel",
      "Direction": "InputOutput",
      "RecordType": "SENSORS_DATA",
      "Technology": "Software",
      "Protocol": "",
      "IsRemote": false
    },
    {
      "Name": "MicBufferDataChannel",
      "Direction": "InputOutput",
      "RecordType": "MIC_BUFFER_DATA",
      "Technology": "Software",
      "Protocol": "",
      "IsRemote": false
    },
    {
      "Name": "MicPeakDataChannel",
      "Direction": "InputOutput",
      "RecordType": "MIC_PEAK_DATA",
      "Technology": "Software",
      "Protocol": "",
      "IsRemote": false
    },
    {
      "Name": "MotionDataChannel",
      "Direction": "InputOutput",
      "RecordType": "MOTION_DATA",
      "Technology": "Software",
      "Protocol": "",
      "IsRemote": false
    }
  ],
  "Topology": [
    {
      "Output": {
        "AIMName": "MotionRecognitionAnalysis",
        "PortName": "SensorsDataChannel"
      },
      "Input": {
        "AIMName": "ControlUnitSensorsReading",
        "PortName": "SensorsDataChannel"
      }
    },
    {
      "Output": {
        "AIMName": "MovementsWithAudioValidation",
        "PortName": "MicPeakDataChannel"
      },
      "Input": {
        "AIMName": "VolumePeaksAnalysis",
        "PortName": "MicPeakDataChannel"
      }
    },
    {
      "Output": {
        "AIMName": "",
        "PortName": "MicBufferDataChannel"
      },
      "Input": {
        "AIMName": "VolumePeaksAnalysis",
        "PortName": ""
      }
    },
    {
      "Output": {
        "AIMName": "MovementsWithAudioValidation",
        "PortName": "MotionDataChannel"
      },
      "Input": {
        "AIMName": "MotionRecognitionAnalysis",
        "PortName": "MotionDataChannel"
      }
    }
  ],
  "SubAIMs": [
    {
      "Name": "VolumePeaksAnalysis",
      "Identifier": {
        "ImplementerID": 1,
        "Specification": {
          "Standard": "MPAI-CAE",
          "AIW": "CAE-REV",
          "AIM": "VolumePeaksAnalysis",
          "Version": "1"
        }
      }
    },
    {
      "Name": "ControlUnitSensorsReading",
      "Identifier": {
        "ImplementerID": 1,
        "Specification": {
          "Standard": "MPAI-CAE",
          "AIW": "CAE-REV",
          "AIM": "ControlUnitSensorsReading",
          "Version": "1"
        }
      }
    },
    {
      "Name": "MotionRecognitionAnalysis",
      "Identifier": {
        "ImplementerID": 1,
        "Specification": {
          "Standard": "MPAI-CAE",
          "AIW": "CAE-REV",
          "AIM": "MotionRecognitionAnalysis",
          "Version": "1"
        }
      }
    },
    {
      "Name": "MovementsWithAudioValidation",
      "Identifier": {
        "ImplementerID": 1,
        "Specification": {
          "Standard": "MPAI-CAE",
          "AIW": "CAE-REV",
          "AIM": "MovementsWithAudioValidation",
          "Version": "1"
        }
      }
    }
  ],
  "Implementations": [
    {
      "BinaryName": "firmware.bin",
      "Architecture": "arm",
      "OperatingSystem": "Zephyr RTOS",
      "Version": "v0.1",
      "Source": "AIMStorage",
      "Destination": ""
    }
  ],
  "ResourcePolicies": [
    {
      "Name": "Memory",
      "Minimum": "50000",
      "Maximum": "120000",
      "Request": "80000"
    },
    {
      "Name": "CPUNumber",
      "Minimum": "1",
      "Maximum": "2",
      "Request": "1"
    },
    {
      "Name": "CPU:Class",
      "Minimum": "Low",
      "Maximum": "High",
      "Request": "Low"
    }
  ],
  "Documentation": [
    {
      "Type": "Tutorial",
      "URI": "https://mpai.community/standards/mpai-cae/"
    }
  ]
}
```

and consists of 4 AIMs:
1. *VolumePeaksAnalysis*: uses microphones to identify audio pattern (in this case volume peaks) and publish it to the channel `MicPeakDataChannel` of the message store
2. *ControlUnitSensorsReading*: reads data from all device sensors (like temperature, acceleration, pressure and others) and publish the values to the channel `SensorsDataChannel` of the message store
3. *MotionRecognitionAnalysis*:  uses data from inertial unit, coming from `SensorsDataChannel`, to detect motion events such as start, stop etc and publish them to channel `MotionDataChannel` of the message store
4. *MovementsWithAudioValidation*: uses data, cross-referencing it between `MotionDataChannel` and `MicPeakDataChannel`, to recognize if the movement is done in a correct way. In particular, detects a stop event and waits for a volume peak at maximum for 1sec (configurable). It also blinks alternately the leds to alert the error.

# INSTALLATION  
1. Install PlatformIO Core [here](http://docs.platformio.org/page/core.html)
2. Download `development platform with examples [here](https://github.com/platformio/platform-ststm32/archive/develop.zip)
3. Extract ZIP archive
4. Run (if requested) [MPAI Server COAP](https://github.com/dbortoluzzi/mpai_store_coap_server)
5. Configure WLAN (if requested), creating a file *wifi_config.c* like below:

```c

    #include "wifi_config.h"

    char* AUTO_CONNECT_SSID = "<SSID>";

    char* AUTO_CONNECT_SSID_PSK = "<PASSWORD>";
```

6. Run these commands:

```bash
    # Change directory to example
    > cd iotnode_box_test_sensors

    # Build project
    > platformio run

    # Upload firmware
    > platformio run --target upload

    # Build specific environment
    > platformio run -e iotnode_box

    # Upload firmware for the specific environment
    > platformio run -e iotnode_box --target upload

    # Clean build files
    > platformio run --target clean
```

# License
License information for each components of this example is detailed in [LICENSE.MD](/LICENSE.md)
