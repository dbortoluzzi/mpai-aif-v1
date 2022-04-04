#ifndef MPAI_LIBS_SENSORS_AIM_H
#define MPAI_LIBS_SENSORS_AIM_H

#include <core_common.h>
#include <core_aim.h>
#include <sensors_common.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <message_store.h>

__weak MPAI_AIM_MessageStore_t* message_store_sensors_aim;
__weak subscriber_channel_t SENSORS_DATA_CHANNEL;

mpai_error_t* sensors_aim_subscriber();

mpai_error_t* sensors_aim_start();

mpai_error_t* sensors_aim_stop();

mpai_error_t* sensors_aim_resume();

mpai_error_t* sensors_aim_pause();

#endif