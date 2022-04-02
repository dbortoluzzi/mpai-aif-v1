#include "rehabilitation_aim.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(MPAI_LIBS_REHABILITATION_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between sensors (in ms) */
#define CONFIG_SENSORS_RATE_MS 100

/*************** STATIC ***************/
static const struct device *led0;

/**************** THREADS **********************/

static k_tid_t subscriber_rehabilitation_thread_id;

K_THREAD_STACK_DEFINE(thread_sub_rehabilitation_stack_area, STACKSIZE);
static struct k_thread thread_sub_rehabilitation_sens_data;

/* SUBSCRIBER */

void th_subscribe_rehabilitation_data(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	mpai_parser_t aim_message;

	LOG_INF("START SUBSCRIBER");

	while (1)
	{
		/* this function will return once new data has arrived, or upon timeout (1000ms in this case). */
		int ret = MPAI_MessageStore_poll(message_store_rehabilitation_aim, rehabilitation_aim_subscriber, K_MSEC(1000), MOTION_DATA_CHANNEL);

		/* ret returns:
		 * a positive value if new data was successfully returned
		 * 0 if the poll timed out
		 * negative if an error occured while polling
		 */
		if (ret > 0)
		{
			MPAI_MessageStore_copy(message_store_rehabilitation_aim, rehabilitation_aim_subscriber, &aim_message);
			LOG_INF("Received from timestamp %lld\n", aim_message.timestamp);

			motion_data_t *motion_data = (motion_data_t *)aim_message.data;

			if (motion_data->motion_type == STOPPED)
			{
				LOG_INF("STOPPATO!!");

			} else if (motion_data->motion_type == STARTED)
			{

			}

		}
		else if (ret == 0)
		{
			printk("WARNING: Did not receive new data for 1000ms. Continuing poll.\n");
		}
		else
		{
			printk("ERROR: error while polling: %d\n", ret);
			return;
		}
	}
}

/************** EXECUTIONS ***************/
mpai_error_t* rehabilitation_aim_subscriber()
{
	// NO-OP
}

mpai_error_t *rehabilitation_aim_start()
{
	// LED
	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
					   GPIO_OUTPUT_ACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

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