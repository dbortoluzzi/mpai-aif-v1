#include "temp_limit_aim.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(MPAI_LIBS_TEMP_LIMIT_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between sensors (in ms) */
#define CONFIG_SENSORS_RATE_MS 1000

#define TEMPERATURE_LIMIT 30.0


/*************** STATIC ***************/
static const struct device *led0;

/**************** THREADS **********************/

static k_tid_t subscriber_thread_id;

K_THREAD_STACK_DEFINE(thread_sub_stack_area, STACKSIZE);
static struct k_thread thread_sub_sens_data;

/* SUBSCRIBER */

void th_subscribe_sensors_data(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	mpai_parser_t aim_message;

	LOG_INF("START SUBSCRIBER");

	while (1)
	{
		LOG_INF("Reading from pubsub......\n\n");

		/* this function will return once new data has arrived, or upon timeout (1000ms in this case). */
		int ret = MPAI_MessageStore_poll(message_store, temp_limit_aim_subscriber, K_MSEC(1000));

		/* ret returns:
		 * a positive value if new data was successfully returned
		 * 0 if the poll timed out
		 * negative if an error occured while polling
		 */
		if (ret > 0)
		{
			MPAI_MessageStore_copy(message_store, temp_limit_aim_subscriber, &aim_message);
			LOG_INF("Received from timestamp %lld\n", aim_message.timestamp);

			/* Display sensor datsa */

			sensor_result_t *sensor_data = (sensor_result_t *)aim_message.data;

			#ifdef CONFIG_HTS221
			/* HTS221 temperature */
			printk("HTS221: Temperature: %.1f C\n",
				   sensor_value_to_double(sensor_data->hts221_temp));

			/* HTS221 humidity */
			printk("HTS221: Relative Humidity: %.1f%%\n",
				   sensor_value_to_double(sensor_data->hts221_hum));
			#endif

			printk("\n");

			#ifdef CONFIG_HTS221
				double actual_temperature = sensor_value_to_double(sensor_data->hts221_temp);
				if (actual_temperature > TEMPERATURE_LIMIT)
				{
					printk("Temperature exceeds limit: %.3f > %.3f\n", actual_temperature, TEMPERATURE_LIMIT);
					gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 1);
				}
				else
				{
					gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 0);
				}
			#endif
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
mpai_error_t* temp_limit_aim_subscriber()
{
	// NO-OP
}

mpai_error_t *temp_limit_aim_start()
{
	// LED
	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
					   GPIO_OUTPUT_ACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

	// CREATE SUBSCRIBER
	subscriber_thread_id = k_thread_create(&thread_sub_sens_data, thread_sub_stack_area,
										 K_THREAD_STACK_SIZEOF(thread_sub_stack_area),
										 th_subscribe_sensors_data, NULL, NULL, NULL,
										 PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_sub_sens_data, "thread_sub");

	// START THREAD
	k_thread_start(subscriber_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *temp_limit_aim_stop()
{
	k_thread_abort(subscriber_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *temp_limit_aim_resume()
{
	k_thread_resume(subscriber_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *temp_limit_aim_pause()
{
	k_thread_suspend(subscriber_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}