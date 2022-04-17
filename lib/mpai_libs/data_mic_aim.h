/*
 * @file
 * @brief Headers of an AIM that reads data from mic and recognized volume peaks
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_DATA_AIM_H
#define MPAI_LIBS_DATA_AIM_H

#include <core_common.h>
#include <drivers/uart.h>
#include <core_common.h>
#include <core_aim.h>
#include <mic_common.h>
#include <stm32l475e_iot01_audio.h>
#include <drivers/gpio.h>
#include <misc_utils.h>

// The implementation will be added in AIW configuration
__weak MPAI_AIM_MessageStore_t* message_store_data_mic_aim;
__weak subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
__weak subscriber_channel_t MIC_PEAK_DATA_CHANNEL;

// AIM subscriber
mpai_error_t* data_mic_aim_subscriber();

// AIM high priorities commands
mpai_error_t* data_mic_aim_start();

mpai_error_t* data_mic_aim_stop();

mpai_error_t* data_mic_aim_resume();

mpai_error_t* data_mic_aim_pause();

#endif