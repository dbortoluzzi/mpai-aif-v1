/*
* @file
 * @brief Headers of an AIM that recognizes the motion of device (Start, Stop, ecc...)
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_MOTION_AIM_H
#define MPAI_LIBS_MOTION_AIM_H

#include <core_common.h>
#include <drivers/uart.h>
#include <core_common.h>
#include <sensors_common.h>
#include <core_aim.h>
#include <motion_common.h>
#include <math.h>

// The implementation will be added in AIW configuration
__weak MPAI_AIM_MessageStore_t* message_store_motion_aim;
__weak subscriber_channel_t MOTION_DATA_CHANNEL;
__weak subscriber_channel_t SENSORS_DATA_CHANNEL;

// AIM subscriber
mpai_error_t* motion_aim_subscriber();

// AIM high priorities commands
mpai_error_t* motion_aim_start();

mpai_error_t* motion_aim_stop();

mpai_error_t* motion_aim_resume();

mpai_error_t* motion_aim_pause();

#endif