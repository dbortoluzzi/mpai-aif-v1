#ifndef MPAI_LIBS_REHABILITATION_AIM_H
#define MPAI_LIBS_REHABILITATION_AIM_H

#include <core_common.h>
#include <drivers/uart.h>
#include <core_common.h>
#include <sensors_common.h>
#include <core_aim.h>
#include <motion_common.h>
#include <math.h>

__weak MPAI_AIM_MessageStore_t* message_store_rehabilitation_aim;
__weak subscriber_channel_t MOTION_DATA_CHANNEL;
__weak subscriber_channel_t MIC_PEAK_DATA_CHANNEL;

mpai_error_t* rehabilitation_aim_subscriber();

mpai_error_t* rehabilitation_aim_start();

mpai_error_t* rehabilitation_aim_stop();

mpai_error_t* rehabilitation_aim_resume();

mpai_error_t* rehabilitation_aim_pause();

#endif