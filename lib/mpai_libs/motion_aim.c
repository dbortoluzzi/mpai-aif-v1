/*
 * Copyright (c) 2022 Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "motion_aim.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(MPAI_LIBS_MOTION_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* min delay used to detect when mcu is stopped */
#define MCU_MIN_DETECTED_STOP_DELAY_MS 100

#define SENSORS_DATA_POLLING_MS 1000

#define ACCEL_TOT_THRESHOLD_MIN 9.5
#define ACCEL_TOT_THRESHOLD_MAX 10.5

/*************** STATIC ***************/
static int64_t mcu_has_stopped_ts = 0.0;
static motion_data_t motion_data = {.motion_type = UNKNOWN};

static void publish_motion_to_message_store(MOTION_TYPE motion_type)
{
	motion_data.motion_type = motion_type;
	
	// Publish sensor message 
	mpai_parser_t msg = {
		.data = &motion_data,
		.timestamp = k_uptime_get()
	};

	MPAI_MessageStore_publish(message_store_motion_aim, &msg, MOTION_DATA_CHANNEL);

	LOG_DBG("Message motion published");
}

/**************** THREADS **********************/

static k_tid_t subscriber_motion_thread_id;

K_THREAD_STACK_DEFINE(thread_sub_motion_stack_area, STACKSIZE);
static struct k_thread thread_sub_motion_sens_data;

void th_subscribe_motion_data(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	mpai_parser_t aim_message;

	LOG_INF("START SUBSCRIBER");

	while (1)
	{
		// LOG_INF("Reading from pubsub......\n\n");

		/* this function will return once new data has arrived, or upon timeout (1000ms in this case). */
		int ret = MPAI_MessageStore_poll(message_store_motion_aim, motion_aim_subscriber, K_MSEC(SENSORS_DATA_POLLING_MS), SENSORS_DATA_CHANNEL);

		/* ret returns:
		 * a positive value if new data was successfully returned
		 * 0 if the poll timed out
		 * negative if an error occured while polling
		 */
		if (ret > 0)
		{
			MPAI_MessageStore_copy(message_store_motion_aim, motion_aim_subscriber, SENSORS_DATA_CHANNEL, &aim_message);
			LOG_DBG("Received from timestamp %lld\n", aim_message.timestamp);

			sensor_result_t *sensor_data = (sensor_result_t *)aim_message.data;

			#ifdef CONFIG_LSM6DSL
				float accel_x = sensor_value_to_double(&(sensor_data->lsm6dsl_accel[0]));
				float accel_y = sensor_value_to_double(&(sensor_data->lsm6dsl_accel[1]));
				float accel_z = sensor_value_to_double(&(sensor_data->lsm6dsl_accel[2]));
				// compute vectorial product to get the total acceleration
				float accel_tot = sqrt(accel_x*accel_x + accel_y*accel_y + accel_z*accel_z);

				// algorithm to check if mcu is stopped or not
				// 1. check if the total acceleration is between the MIN and the MAX threshold
				if (accel_tot >= ACCEL_TOT_THRESHOLD_MIN && accel_tot <= ACCEL_TOT_THRESHOLD_MAX)
				{
					if (mcu_has_stopped_ts != 0 && aim_message.timestamp - mcu_has_stopped_ts >=  MCU_MIN_DETECTED_STOP_DELAY_MS)
					{
						// MCU is stopped but no publish event	
					}
					// 2. check if mcu was moving
					else if (mcu_has_stopped_ts == 0)
					{
						mcu_has_stopped_ts = aim_message.timestamp;	

						printk("MCU Motion stopped: Accel (m.s-2): tot: %.5f\n", accel_tot);

						publish_motion_to_message_store(STOPPED);
					}
				} else 
				{	
					if (mcu_has_stopped_ts > 0) 
					{
						printk("MCU Motion started: Accel (m.s-2): tot: %.5f\n", accel_tot);

						// publish_motion_to_message_store(STARTED);
					}
					mcu_has_stopped_ts = 0;
				}
			#endif
		}
		else if (ret == 0)
		{
			printk("WARNING: Did not receive new data for %dms. Continuing poll.\n", SENSORS_DATA_POLLING_MS);
		}
		else
		{
			printk("ERROR: error while polling: %d\n", ret);
			return;
		}
	}
}

/************** EXECUTIONS ***************/
mpai_error_t* motion_aim_subscriber()
{
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *motion_aim_start()
{
	mcu_has_stopped_ts = k_uptime_get();

	// CREATE SUBSCRIBER
	subscriber_motion_thread_id = k_thread_create(&thread_sub_motion_sens_data, thread_sub_motion_stack_area,
										 K_THREAD_STACK_SIZEOF(thread_sub_motion_stack_area),
										 th_subscribe_motion_data, NULL, NULL, NULL,
										 PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_sub_motion_sens_data, "thread_sub_motion");

	// START THREAD
	k_thread_start(subscriber_motion_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *motion_aim_stop()
{
	k_thread_abort(subscriber_motion_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *motion_aim_resume()
{
	k_thread_resume(subscriber_motion_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *motion_aim_pause()
{
	k_thread_suspend(subscriber_motion_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}