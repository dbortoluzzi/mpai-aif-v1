MPAI- AIF v1.0 implementation for the ST IoT NODE L-L475E-IOT01A, (based on PlatformIO)
=====================================
This code refers to the first implementation of the MPAI-AIF specification as described in https://mpai.community/wp-content/uploads/2021/10/MPAI-AIF-WD0.12.pdf. It contains a given number of AI modules (AIMs) and implements a simple use case.

The software runs on the ST IoT NODE https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html

The Architecture has the following characteristics
•	based on Zephyr operating system (based on RTOS)
•	Implements only MPAI Events (High Priority events)
•	Implements a message store according to the MPAI-AIF 1.0 spec
•	Implements great part the MPAI APIs in the form of libraries
•	Implements a communication interface via IP based on CoAP  (Constrained Application Protocol) that simulates the MPAI Store.
•	Parses json files according to the MAPI-AIF V 1.0 specification
•	All the code is written in C language.
 
In order to test the architecture few test AIMs have been implemented:

AIM Producers  
•	Read data from sensors 
•	Does some level of processing
•	Produces an output as a message
•	Pass the message on a message store and make it ready for consumption 

AIM Consumers
•	Reads data as message from the message store produce an output



BRIEF DESCRIPTION OF USE CASE
=====================================    

A use case for testing the MPAI-AIF implementation has been identified. We call this use case the rehabilitation UC in which specific movements need to be performed in sync with the audio clue. The system listens via the ASC AIM to the audio signal for specific patterns (i.e., a low frequency impulse coming from a metronome) and simultaneously monitor the movement patterns via the Human Activity Classification HAC AIM.  An additional AIM monitors if the movement is detected as correct and executed in synchronization with the spe-cific audio pattern. In affirmative case outputs a message via the serial port AIM. 



INSTALLATION
=====================================    
1. `Install PlatformIO Core <http://docs.platformio.org/page/core.html>`_
2. Download `development platform with examples <https://github.com/platformio/platform-ststm32/archive/develop.zip>`_
3. Extract ZIP archive
4. Run (if requested) `MPAI Server COAP <https://github.com/dbortoluzzi/mpai_store_coap_server>`_
5. Configure WLAN (if requested), creating a file *wifi_config.c* like below:

.. code-block:: c

    #include "wifi_config.h"

    char* AUTO_CONNECT_SSID = "<SSID>";

    char* AUTO_CONNECT_SSID_PSK = "<PASSWORD>";


6. Run these commands:

.. code-block:: bash

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
    
USAGE    
=====================================    

License
=====================================    
License information for each components of this example is detailed in <LICENCE.md>
