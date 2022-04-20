How to build the project for the ST IOTNODE, based on PlatformIO
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
    

License
=====================================    
License information for each components of this example is detailed in <LICENCE.md>
