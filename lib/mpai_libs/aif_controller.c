/*
 * @file
 * @brief Implementation of a MPAI AIF Controller, according to the specs V1
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aif_controller.h"

LOG_MODULE_REGISTER(MPAI_LIBS_AIF_CONTROLLER, LOG_LEVEL_INF);

#include <wifi_connect.h>
#include <net_private.h>

/* AIMs to configured */
MPAI_Component_AIM_t* aim_produce_sensors = NULL;
MPAI_Component_AIM_t* aim_temp_limit = NULL;
MPAI_Component_AIM_t* aim_data_mic = NULL;
MPAI_Component_AIM_t* aim_data_motion = NULL;
MPAI_Component_AIM_t* aim_rehabilitation = NULL;

#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC

/******** START PERIODIC MODE ***********/
void aim_timer_switch_status(struct k_work *work)
{
	if (MPAI_AIM_Is_Alive(aim_produce_sensors) == true) {
		MPAI_AIM_Pause(aim_produce_sensors);
	} else {
		MPAI_AIM_Resume(aim_produce_sensors);
	}
}

K_WORK_DEFINE(my_work, aim_timer_switch_status);

void aim_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(aim_timer, aim_timer_handler, NULL);
/******** END PERIODIC MODE ***********/
#endif	

/*** START COAP ***/
#ifdef CONFIG_COAP_SERVER
	const char * const test_path[] = { "test", NULL };

	const char * const large_path[] = { "large", NULL };

	const char * const obs_path[] = { "obs", NULL };

	static int send_simple_coap_msgs_and_wait_for_reply(uint8_t * data_result, const char * const * simple_path)
	{
		uint8_t test_type = 0U;
		int r;

		while (1) {
			switch (test_type) {
			case 0:
				/* Test CoAP GET method */
				printk("\nCoAP client GET\n");
				r = send_simple_coap_request(COAP_METHOD_GET, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			case 1:
				/* Test CoAP PUT method */
				printk("\nCoAP client PUT\n");
				r = send_simple_coap_request(COAP_METHOD_PUT, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			case 2:
				/* Test CoAP POST method*/
				printk("\nCoAP client POST\n");
				r = send_simple_coap_request(COAP_METHOD_POST, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			case 3:
				/* Test CoAP DELETE method*/
				printk("\nCoAP client DELETE\n");
				r = send_simple_coap_request(COAP_METHOD_DELETE, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			default:
				return 0;
			}

			memset(data_result, 0, MAX_COAP_MSG_LEN);
			r = process_simple_coap_reply(data_result);
			if (r < 0) {
				return r;
			}
			test_type++;
		}

		return 0;
	}

	static int send_obs_reply_ack(uint16_t id, uint8_t *token, uint8_t tkl)
	{
		struct coap_packet request;
		uint8_t *data;
		int r;

		data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
		if (!data) {
			return -ENOMEM;
		}

		r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
					COAP_VERSION_1, COAP_TYPE_ACK, tkl, token, 0, id);
		if (r < 0) {
			LOG_ERR("Failed to init CoAP message");
			goto end;
		}

		net_hexdump("Request", request.data, request.offset);

		r = send(get_coap_sock(), request.data, request.offset, 0);
	end:
		k_free(data);

		return r;
	}

	static int send_obs_coap_request(void)
	{
		struct coap_packet request;
		const char * const *p;
		uint8_t *data;
		int r;

		data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
		if (!data) {
			return -ENOMEM;
		}

		r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
					COAP_VERSION_1, COAP_TYPE_CON,
					COAP_TOKEN_MAX_LEN, coap_next_token(),
					COAP_METHOD_GET, coap_next_id());
		if (r < 0) {
			LOG_ERR("Failed to init CoAP message");
			goto end;
		}

		r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
		if (r < 0) {
			LOG_ERR("Failed to append Observe option");
			goto end;
		}

		for (p = obs_path; p && *p; p++) {
			r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
							*p, strlen(*p));
			if (r < 0) {
				LOG_ERR("Unable add option to request");
				goto end;
			}
		}

		net_hexdump("Request", request.data, request.offset);

		r = send(get_coap_sock(), request.data, request.offset, 0);

	end:
		k_free(data);

		return r;
	}

	static int send_obs_reset_coap_request(void)
	{
		struct coap_packet request;
		const char * const *p;
		uint8_t *data;
		int r;

		data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
		if (!data) {
			return -ENOMEM;
		}

		r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
					COAP_VERSION_1, COAP_TYPE_RESET,
					COAP_TOKEN_MAX_LEN, coap_next_token(),
					0, coap_next_id());
		if (r < 0) {
			LOG_ERR("Failed to init CoAP message");
			goto end;
		}

		r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
		if (r < 0) {
			LOG_ERR("Failed to append Observe option");
			goto end;
		}

		for (p = obs_path; p && *p; p++) {
			r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
							*p, strlen(*p));
			if (r < 0) {
				LOG_ERR("Unable add option to request");
				goto end;
			}
		}

		net_hexdump("Request", request.data, request.offset);

		r = send(get_coap_sock(), request.data, request.offset, 0);

	end:
		k_free(data);

		return r;
	}

	static int register_observer(void)
	{
		uint8_t counter = 0U;
		int r;

		while (1) {
			/* Test CoAP OBS GET method */
			if (!counter) {
				printk("\nCoAP client OBS GET\n");
				r = send_obs_coap_request();
				if (r < 0) {
					return r;
				}
			} else {
				printk("\nCoAP OBS Notification\n");
			}

			r = process_obs_coap_reply();
			if (r < 0) {
				return r;
			}

			counter++;

			/* Unregister */
			if (counter == 5U) {
				/* TODO: Functionality can be verified byt waiting for
				* some time and make sure client shouldn't receive
				* any notifications. If client still receives
				* notifications means, Observer is not removed.
				*/
				return send_obs_reset_coap_request();
			}
		}

		return 0;
	}
	//*** END COAP ***/
#endif

mpai_error_t MPAI_AIFU_Controller_Initialize()
{

	#ifdef CONFIG_APP_TEST_WRITE_TO_FLASH
		/*** START SPI FLASH ***/
		const char expected[] = "{\"name\":\"Daniele\"}";
		const size_t len = sizeof(expected);
		char buf[sizeof(expected)];

		const struct device* flash_dev = init_flash();

		LOG_INF("Test 1: Flash erase\n");
		erase_flash(flash_dev);

		LOG_INF("Test 2: Flash write\n");
		int rc_write = write_flash(flash_dev, len, (void*) expected);
		if (rc_write != 0) 
		{
			return;
		}

		int rc_read = read_flash(flash_dev, len, (void*) buf);
		if (rc_read != 0)
		{
			return;
		}
		if (memcmp(expected, buf, len) == 0) 
		{
			LOG_INF("Data read matches data written. Good!!\n");
		} else 
		{
			const char* wp = expected;
			const char* rp = buf;
			const char* rpe = rp + len;

			LOG_ERR("Data read does not match data written!!\n");
			while (rp < rpe) {
				LOG_ERR("%08x wrote %02x read %02x %s\n",
					(uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
					*wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
				++rp;
				++wp;
			}
		}
		JSON_Value* json = json_parse_string(buf);
		const JSON_Object* root_json = json_object(json);
		const char* name = json_object_get_string(root_json, "name");
		LOG_INF("Hello, %s.", log_strdup(name));
		json_value_free(json);
		/*** END SPI FLASH ***/
	#endif

	wifi_connect();

	#ifdef CONFIG_COAP_SERVER
		/*** START COAP ***/
		int r;

		LOG_DBG("Start CoAP-client sample");
		r = start_coap_client();
		if (r < 0) {
			(void)close(get_coap_sock());
		}

		// /* GET, PUT, POST, DELETE */
		uint8_t* data_result = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN * sizeof(uint8_t));
		r = send_simple_coap_msgs_and_wait_for_reply(data_result, test_path);
		k_free(data_result);

		/* Block-wise transfer */
		char* data_large_result = get_large_coap_msgs(large_path);
		k_free(data_large_result);

		/* Register observer, get notifications and unregister */
		// r = register_observer();
		// if (r < 0) {
		// 	(void)close(get_coap_sock());
		// }
	#endif

	#if defined(CONFIG_MPAI_CONFIG_STORE) && defined (CONFIG_MPAI_CONFIG_STORE_USES_COAP)
		char* aif_result = MPAI_Config_Store_Get_AIF("demo");
		if (aif_result != NULL) {
			printk("AIF RESULT: \n");
			for ( size_t i = 0; i < strlen(aif_result); i++ )
			{
				printk("%c", (char)aif_result[i]);
				k_sleep(K_MSEC(5));
			}
			printk("\n");

			JSON_Value* json2 = json_parse_string(aif_result);
			char* name2 = json_object_get_string(json_object(json2), "title");
			LOG_INF("Initializing AIF with title \"%s\"...", log_strdup(name2));

			// TODO: AIF initialization........

			k_free(aif_result);
		}

		/* Close the socket when it's no longer usefull*/
		(void)close(get_coap_sock());
	#endif

	INIT_Test_Use_Case_AIW();

	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		aim_data_mic = MPAI_AIM_Creator("AIM_DATA_MIC", AIW_USE_CASE_ID, data_mic_aim_subscriber, data_mic_aim_start, data_mic_aim_stop, data_mic_aim_resume, data_mic_aim_pause);
		mpai_error_t err_data_mic = MPAI_AIM_Start(aim_data_mic);	

		if (err_data_mic.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_data_mic)->name), log_strdup(MPAI_ERR_STR(err_data_mic.code)));
			return;
		} 
	#endif
	
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		aim_produce_sensors = MPAI_AIM_Creator("AIM_PRODUCE_SENSORS_DATA", AIW_USE_CASE_ID, sensors_aim_subscriber, sensors_aim_start, sensors_aim_stop, sensors_aim_resume, sensors_aim_pause);
		mpai_error_t err_sens_aim = MPAI_AIM_Start(aim_produce_sensors);

		if (err_sens_aim.code != MPAI_AIF_OK) 
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_produce_sensors)->name), log_strdup(MPAI_ERR_STR(err_sens_aim.code)));
			return;
		}
	#endif

	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		aim_temp_limit = MPAI_AIM_Creator("AIM_TEMP_LIMIT", AIW_USE_CASE_ID, temp_limit_aim_subscriber, temp_limit_aim_start, temp_limit_aim_stop, temp_limit_aim_resume, temp_limit_aim_pause);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_temp_limit), SENSORS_DATA_CHANNEL);
		mpai_error_t err_temp_limit = MPAI_AIM_Start(aim_temp_limit);	

		if (err_temp_limit.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_temp_limit)->name), log_strdup(MPAI_ERR_STR(err_temp_limit.code)));
			return;
		} 
	#endif

	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		aim_data_motion = MPAI_AIM_Creator("AIM_MOTION", AIW_USE_CASE_ID, motion_aim_subscriber, motion_aim_start, motion_aim_stop, motion_aim_resume, motion_aim_pause);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_data_motion), SENSORS_DATA_CHANNEL);
		mpai_error_t err_motion = MPAI_AIM_Start(aim_data_motion);	

		if (err_motion.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_data_motion)->name), log_strdup(MPAI_ERR_STR(err_motion.code)));
			return;
		} 
	#endif

	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		aim_rehabilitation = MPAI_AIM_Creator("AIM_REHABILITATION", AIW_USE_CASE_ID, rehabilitation_aim_subscriber, rehabilitation_aim_start, rehabilitation_aim_stop, rehabilitation_aim_resume, rehabilitation_aim_pause);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MOTION_DATA_CHANNEL);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MIC_PEAK_DATA_CHANNEL);
		mpai_error_t err_rehabilitation = MPAI_AIM_Start(aim_rehabilitation);	

		if (err_rehabilitation.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_rehabilitation)->name), log_strdup(MPAI_ERR_STR(err_rehabilitation.code)));
			return;
		} 
	#endif

	LOG_INF("MPAI_AIF initialized correctly");

	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC
		/* start periodic timer to switch status */
		k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));		
	#endif

	k_sleep(K_SECONDS(2));

	MPAI_AIFU_Controller_Destroy();

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIFU_Controller_Destroy() 
{
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIM_Stop(aim_rehabilitation);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIM_Stop(aim_data_motion);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIM_Stop(aim_temp_limit);
	#endif
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIM_Stop(aim_produce_sensors);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Stop(aim_data_mic);
	#endif

	k_sleep(K_SECONDS(2));

	DESTROY_Test_Use_Case_AIW();

	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIM_Destructor(aim_rehabilitation);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIM_Destructor(aim_data_motion);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIM_Destructor(aim_temp_limit);
	#endif
	MPAI_AIM_Destructor(aim_produce_sensors);
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Destructor(aim_data_mic);
	#endif
	
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}
