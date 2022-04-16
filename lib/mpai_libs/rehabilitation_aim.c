/*
 * Copyright (c) 2022 Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rehabilitation_aim.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(MPAI_LIBS_REHABILITATION_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* polling every XXX(ms) to check new motion messages */
#define CONFIG_REHABILITATION_MOTION_TIMEOUT_MS 3000
/* polling every XXX(ms) to check new volume peak messages */
#define CONFIG_REHABILITATION_MIC_PEAK_TIMEOUT_MS 1000

/*************** STATIC ***************/
static const struct device *led0, *led1;

/**************** THREADS **********************/

static k_tid_t subscriber_rehabilitation_thread_id;

K_THREAD_STACK_DEFINE(thread_sub_rehabilitation_stack_area, STACKSIZE);
static struct k_thread thread_sub_rehabilitation_sens_data;

/* SUBSCRIBER */

void show_movement_error()
{
	// Show error blinking leds
	int i, on = 1;
	for (i = 0; i < 6; i++)
	{
		gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), on);
		gpio_pin_set(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios), !on);
		k_sleep(K_MSEC(100));
		on = (on == 1) ? 0 : 1;
	}
}

void th_subscribe_rehabilitation_data(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	mpai_parser_t aim_motion_message;

	LOG_INF("START SUBSCRIBER");

	while (1)
	{
		int ret_motion = MPAI_MessageStore_poll(message_store_rehabilitation_aim, rehabilitation_aim_subscriber, K_MSEC(CONFIG_REHABILITATION_MOTION_TIMEOUT_MS), MOTION_DATA_CHANNEL);

		if (ret_motion > 0)
		{
			MPAI_MessageStore_copy(message_store_rehabilitation_aim, rehabilitation_aim_subscriber, MOTION_DATA_CHANNEL, &aim_motion_message);
			LOG_DBG("Received from timestamp %lld\n", aim_motion_message.timestamp);

			motion_data_t *motion_data = (motion_data_t *)aim_motion_message.data;

			if (motion_data->motion_type == STOPPED)
			{
				LOG_INF("MOTION STOPPED: Waiting for Audio Peak");
				k_sleep(K_MSEC(10));
				int ret_mic = MPAI_MessageStore_poll(message_store_rehabilitation_aim, rehabilitation_aim_subscriber, K_MSEC(CONFIG_REHABILITATION_MIC_PEAK_TIMEOUT_MS), MIC_PEAK_DATA_CHANNEL);

				if (ret_mic > 0)
				{
					LOG_INF("MOVEMENT CORRECT!");
				}
				else if (ret_mic == 0)
				{
					LOG_ERR("MOVEMENT NOT CORRECT! Audio Peak NOT FOUND");

					show_movement_error();
				}
				else
				{
					printk("ERROR: error while polling: %d\n", ret_mic);
					return;
				}
			} else if (motion_data->motion_type == STARTED)
			{	
				// TODO
			}

		}
		else if (ret_motion == 0)
		{
			printk("WARNING: Did not receive new motion data for %d. Continuing poll.\n", CONFIG_REHABILITATION_MOTION_TIMEOUT_MS);
			LOG_WRN("MOVEMENT NOT RECOGNIZED");
			show_movement_error();
		}
		else
		{
			printk("ERROR: error while polling: %d\n", ret_motion);
			return;
		}
	}
}

/************** EXECUTIONS ***************/
mpai_error_t* rehabilitation_aim_subscriber()
{
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *rehabilitation_aim_start()
{
	// LED
	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
					   GPIO_OUTPUT_ACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

	led1 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led1), gpios));
	gpio_pin_configure(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios),
					   GPIO_OUTPUT_INACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led1), gpios));

	// CREATE SUBSCRIBER
	subscriber_rehabilitation_thread_id = k_thread_create(&thread_sub_rehabilitation_sens_data, thread_sub_rehabilitation_stack_area,
										 K_THREAD_STACK_SIZEOF(thread_sub_rehabilitation_stack_area),
										 th_subscribe_rehabilitation_data, NULL, NULL, NULL,
										 PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_sub_rehabilitation_sens_data, "thread_sub_rehabilitation");

	// START THREAD
	k_thread_start(subscriber_rehabilitation_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *rehabilitation_aim_stop()
{
	k_thread_abort(subscriber_rehabilitation_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *rehabilitation_aim_resume()
{
	k_thread_resume(subscriber_rehabilitation_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *rehabilitation_aim_pause()
{
	k_thread_suspend(subscriber_rehabilitation_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}