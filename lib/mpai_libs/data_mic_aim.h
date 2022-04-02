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

__weak MPAI_AIM_MessageStore_t* message_store_data_mic_aim;
__weak subscriber_channel_t MIC_CHANNEL;

mpai_error_t* data_mic_aim_subscriber();

mpai_error_t* data_mic_aim_start();

mpai_error_t* data_mic_aim_stop();

mpai_error_t* data_mic_aim_resume();

mpai_error_t* data_mic_aim_pause();

#endif